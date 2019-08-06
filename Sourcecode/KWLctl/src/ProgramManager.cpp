/*
 * Copyright (C) 2018 Ivan Schréter (schreter@gmx.net)
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 *
 * This copyright notice MUST APPEAR in all copies of the software!
 */

#include "ProgramManager.h"
#include "KWLConfig.h"
#include "FanControl.h"
#include "MQTTTopic.hpp"
#include "StringView.h"

#include <MicroNTP.h>

/// Check current program every 5s.
static constexpr unsigned long PROGRAM_INTERVAL = 5000000;

ProgramManager::ProgramManager(KWLPersistentConfig& config, FanControl& fan, const MicroNTP& ntp) :
  MessageHandler(F("ProgramManager")),
  config_(config),
  fan_(fan),
  ntp_(ntp),
  stats_(F("ProgramManager")),
  timer_task_(stats_, &ProgramManager::run, *this)
{}

void ProgramManager::begin()
{
  timer_task_.runRepeated(PROGRAM_INTERVAL);
}

const ProgramData& ProgramManager::getProgram(unsigned index)
{
  return config_.getProgram(index);
}

void ProgramManager::setProgram(unsigned index, const ProgramData& program)
{
  if (index > KWLConfig::MaxProgramCount)
    return; // ERROR
  config_.setProgram(index, program);
  if (index == unsigned(current_program_))
    current_program_ = -1;
  run();
  publishProgram(index);
  publishProgramIndex();
}

void ProgramManager::enableProgram(unsigned index, uint8_t progsetmask)
{
  if (index > KWLConfig::MaxProgramCount)
    return; // ERROR
  config_.enableProgram(index, progsetmask);
  if (index == unsigned(current_program_))
    current_program_ = -1;
  run();
  publishProgram(index);
}

void ProgramManager::run()
{
  // TODO handle additional input, like humidity sensor

  if (!ntp_.hasTime()) {
    if (KWLConfig::serialDebugProgram)
      Serial.println(F("PROG: check - no time"));
    return;
  }
  auto time = ntp_.currentTimeHMS(config_.getTimezoneMin() * 60L, config_.getDST());
  auto set_index = config_.getProgramSetIndex();
  if (KWLConfig::serialDebugProgram) {
    Serial.print(F("PROG: check at "));
    Serial.print(PrintableHMS(time));
    Serial.print(F(", set index "));
    Serial.println(set_index);
  }

  // iterate all programs and pick one which hits
  int8_t program = -1;
  uint8_t setmask = uint8_t(1 << set_index);
  for (int8_t i = 0; i < KWLConfig::MaxProgramCount; ++i) {
    auto& p = config_.getProgram(unsigned(i));
    if (p.is_enabled(setmask) && p.matches(time)) {
      program = i;
    }
  }
  if (program != current_program_) {
    // switch programs
    if (KWLConfig::serialDebugProgram) {
      Serial.print(F("PROG: new program "));
      Serial.print(program);
      Serial.print(F(" active, previous program "));
      Serial.print(current_program_);
      Serial.print(F(", set mode="));
    }
    if (program >= 0) {
      auto& prog = config_.getProgram(unsigned(program));
      if (KWLConfig::serialDebugProgram)
        Serial.println(prog.fan_mode_);
      fan_.setVentilationMode(prog.fan_mode_);
    } else {
      // TODO what default to use if no program set? It should be last user
      // settings outside of the program. Or default. For now, set default.
      if (KWLConfig::serialDebugProgram)
        Serial.println(KWLConfig::StandardKwlMode);
      fan_.setVentilationMode(KWLConfig::StandardKwlMode);
    }
    current_program_ = program;
    publishProgramIndex();
  }
}

bool ProgramData::matches(HMS hms) const
{
  HMS start(start_h_, start_m_);
  HMS end(end_h_, end_m_);
  uint8_t wd_bit = uint8_t(1 << hms.wd);
  if (start.compareTime(end) > 0) {
    // program crossing midnight
    if (hms.compareTime(start) >= 0) {
      // day 1
      if ((weekdays_ & wd_bit) == 0)
        return false; // wrong day
    } else if (hms.compareTime(end) < 0) {
      // day 2, we need to check weekday of the start day (i.e., previous one)
      wd_bit <<= 1;
      if (wd_bit == 0x80) // overflow
        wd_bit = 1;
      if ((weekdays_ & wd_bit) == 0)
        return false; // wrong day
    } else {
      return false; // out of range
    }
  } else {
    // normal program
    if (hms.compareTime(start) < 0 || hms.compareTime(end) >= 0)
      return false; // out of range
    if ((weekdays_ & wd_bit) == 0)
      return false; // wrong day
  }
  // program matches
  return true;
}

bool ProgramManager::mqttReceiveMsg(const StringView& topic, const StringView& s)
{
  if (topic == MQTTTopic::CmdSetProgramSet) {
    // set program index
    auto set = s.toInt();
    if (set < 0 || set > 7) {
      if (KWLConfig::serialDebugProgram)
        Serial.println(F("PROG: Invalid program set index"));
    } else {
      config_.setProgramSetIndex(uint8_t(set));
      run();  // to pick proper program, if any change
      publishProgramIndex();
    }
    return true;
  }
  if (topic.substr(0, MQTTTopic::CmdSetProgram.length()) != MQTTTopic::CmdSetProgram) {
    // wrong topic
    return false;
  }
  // strip the command and decode index
  auto command = topic.c_str() + MQTTTopic::CmdSetProgram.length();
  auto end = strchr(command, '/');
  if (!end) {
    // wrong topic
    return false;
  }
  char* parse_end;
  auto index = unsigned(strtoul(command, &parse_end, 10));
  bool valid_index = (parse_end == end) && (end != command) && (index < KWLConfig::MaxProgramCount);
  StringView index_str(command, size_t(end - command));
  StringView command_str(end + 1);

  if (command_str == MQTTTopic::SubtopicProgramGet) {
    // get specified program or ALL
    if (index_str == F("all")) {
      // send all programs
      unsigned i = 0;
      bool all = false;
      publisher_.publish([this, i, all]() mutable {
        while (i < KWLConfig::MaxProgramCount) {
          if (!mqttSendProgram(i, all))
            return false; // continue next time
          ++i;
          all = false;
        }
        return true;  // all sent
      });
    } else if (valid_index) {
      publishProgram(index);
    } else {
      if (KWLConfig::serialDebugProgram)
        Serial.println(F("PROG: Invalid program index"));
    }
    publishProgramIndex();
  } else if (command_str == MQTTTopic::SubtopicProgramData) {
    // Parse program string "HH:MM HH:MM F wwwwwww pppppppp", where
    // F is fan mode, wwwwwww are flags for weekdays indicating whether to run
    // the program on a given weekday (0 or 1) and pppppppp are program sets
    // in which to consider the program.
    // Weekday and program sets flags are optional, if not set, run every day
    // and in every program set.
    unsigned start_h, start_m, end_h, end_m, mode;
    char wd_buf[8], ps_buf[9];
    char FORMAT[] = "%u:%u %u:%u %u %7s %8s";
    int rc = sscanf(s.c_str(), FORMAT,
                    &start_h, &start_m, &end_h, &end_m, &mode, wd_buf, ps_buf);
    if (rc < 5 || !valid_index) {
      if (KWLConfig::serialDebugProgram) {
        Serial.print(F("PROG: Invalid program string or program index, parsed items "));
        Serial.print(rc);
        Serial.print('/');
        Serial.println('7');
      }
      return true;
    }
    ProgramData prog;
    if (start_h > 23 || start_m > 59) {
      if (KWLConfig::serialDebugProgram)
        Serial.println(F("PROG: Invalid start time"));
      return true;
    }
    prog.start_h_ = uint8_t(start_h);
    prog.start_m_ = uint8_t(start_m);
    if (end_h > 23 || end_m > 59) {
      if (KWLConfig::serialDebugProgram)
        Serial.println(F("PROG: Invalid end time"));
      return true;
    }
    prog.end_h_ = uint8_t(end_h);
    prog.end_m_ = uint8_t(end_m);
    if (mode >= KWLConfig::StandardModeCnt) {
      if (KWLConfig::serialDebugProgram)
        Serial.println(F("PROG: Invalid mode"));
      return true;
    }
    prog.fan_mode_ = uint8_t(mode);
    if (rc >= 6) {
      prog.weekdays_ = 0;
      const char* p = wd_buf;
      const char* e = p + 7;
      for (uint8_t bit = 1; p < e; bit <<= 1, ++p) {
        switch (*p) {
        case '0':
          break;
        case '1':
          prog.weekdays_ |= bit;
          break;
        default:
          // invalid string
          if (KWLConfig::serialDebugProgram)
            Serial.println(F("PROG: Weekdays must be [01]{7}"));
          return true;
        }
      }
    } else {
      // all weekdays
      prog.weekdays_ = 0x7f;
    }
    if (rc >= 7) {
      prog.enabled_progsets_ = 0;
      const char* p = ps_buf;
      const char* e = p + 8;
      for (uint8_t bit = 1; p < e; bit <<= 1, ++p) {
        switch (*p) {
        case '0':
          break;
        case '1':
          prog.enabled_progsets_ |= bit;
          break;
        default:
          // invalid string
          if (KWLConfig::serialDebugProgram)
            Serial.println(F("PROG: Program set mask must be [01]{8}"));
          return true;
        }
      }
    } else {
      // all program sets
      prog.enabled_progsets_ = 0xff;
    }
    prog.reserved_ = 0;
    setProgram(index, prog);
  } else if (command_str == MQTTTopic::SubtopicProgramEnable) {
    // enable or disable a program
    if (valid_index) {
      const char* p = s.c_str();
      const char* e = p + 8;
      uint8_t progset = 0;
      for (uint8_t bit = 1; p < e; bit <<= 1, ++p) {
        switch (*p) {
        case '0':
          break;
        case '1':
          progset |= bit;
          break;
        default:
          // invalid string
          if (KWLConfig::serialDebugProgram)
            Serial.println(F("PROG: Program set mask must be [01]{8}"));
          return true;
        }
      }
      enableProgram(index, progset);
    } else {
      if (KWLConfig::serialDebugProgram)
        Serial.println(F("PROG: Invalid program index"));
    }
  } else {
    return false;
  }
  return true;
}

void ProgramManager::publishProgram(unsigned index)
{
  bool all = false;
  publisher_.publish([this, index, all]() mutable { return mqttSendProgram(index, all); });
}

void ProgramManager::publishProgramIndex()
{
  int8_t program = current_program_;
  uint8_t set = config_.getProgramSetIndex();
  uint8_t state = 0;
  prognum_publisher_.publish([program, set, state]() mutable {
    if (state == 0) {
      if (publish(MQTTTopic::KwlProgramIndex, program))
        state = 1;
      return false;
    } else if (state == 1) {
      return publish(MQTTTopic::KwlProgramSet, set);
    }
    return true;
  });
}

bool ProgramManager::mqttSendProgram(unsigned index, bool& all)
{
  if (index >= KWLConfig::MaxProgramCount)
    return true;
  auto& prog = config_.getProgram(index);

  static constexpr size_t len = MQTTTopic::KwlProgramData.length();
  char topic[len + 8];
  MQTTTopic::KwlProgramData.store(topic);
  char* pt = topic + len;
  *pt++ = char(index / 10) + '0';
  *pt++ = (index % 10) + '0';
  *pt++ = '/';

  if (all) {
    MQTTTopic::SubtopicProgramEnable.store(pt);
    // Send enabled flags only
    char buf[9];
    char* p = buf;
    for (uint8_t bit = 1; bit != 0; bit <<= 1)
      *p++ = (prog.enabled_progsets_ & bit) ? '1' : '0';
    *p = 0;
    return publish(topic, buf, KWLConfig::RetainProgram);
  } else {
    MQTTTopic::SubtopicProgramData.store(pt);
    // Build program string "HH:MM HH:MM M wwwwwww pppppppp"
    char buffer[32];
    HMS(prog.start_h_, prog.start_m_).writeHM(buffer);
    buffer[5] = ' ';
    HMS(prog.end_h_, prog.end_m_).writeHM(buffer + 6);
    buffer[11] = ' ';
    buffer[12] = char(prog.fan_mode_ + '0');
    buffer[13] = ' ';
    char* p = buffer + 14;
    for (uint8_t bit = 1; bit < 0x80; bit <<= 1)
      *p++ = (prog.weekdays_ & bit) ? '1' : '0';
    *p++ = ' ';
    for (uint8_t bit = 1; bit != 0; bit <<= 1)
      *p++ = (prog.enabled_progsets_ & bit) ? '1' : '0';
    *p = 0;

    all = publish(topic, buffer, KWLConfig::RetainProgram);
    return false; // we need to send enable flag
  }
}
