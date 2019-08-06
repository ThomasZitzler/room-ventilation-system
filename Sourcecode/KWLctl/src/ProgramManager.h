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

/*!
 * @file
 * @brief Program manager.
 */
#pragma once

#include "TimeScheduler.h"
#include "MessageHandler.h"
#include "ProgramData.h"

class KWLPersistentConfig;
class FanControl;
class MicroNTP;

/*!
 * @brief Program manager.
 */
class ProgramManager : private MessageHandler
{
public:
  ProgramManager(const ProgramManager&) = delete;
  ProgramManager& operator=(const ProgramManager&) = delete;

  ProgramManager(KWLPersistentConfig& config, FanControl& fan, const MicroNTP& ntp);

  /// Start the program manager.
  void begin();

  /// Get current program index or <0 if none running.
  int8_t getCurrentProgram() const noexcept { return current_program_; }

  /// Reset program, so the next run will choose current program.
  void resetProgram() { current_program_ = -1; }

  /// Get program data for a given slot.
  const ProgramData& getProgram(unsigned index);

  /// Set program data for a given slot.
  void setProgram(unsigned index, const ProgramData& program);

  /// Enable or disable program for a given slot.
  void enableProgram(unsigned index, uint8_t progsetmask);

private:
  void run();

  virtual bool mqttReceiveMsg(const StringView& topic, const StringView& s) override;

  /// Publish program data via MQTT.
  void publishProgram(unsigned index);

  /// Publish program index and program set index via MQTT.
  void publishProgramIndex();

  /// Send program data via MQTT.
  bool mqttSendProgram(unsigned index, bool& all);

  KWLPersistentConfig& config_;     ///< Persistent configuration.
  FanControl& fan_;                 ///< Fan control to set mode.
  const MicroNTP& ntp_;             ///< Time service.
  int8_t current_program_ = -2;     ///< Index of currently-running program (-2 to force communicating on first run).
  PublishTask publisher_;           ///< Task to publish program data.
  PublishTask prognum_publisher_;   ///< Task to publish program number.
  Scheduler::TaskTimingStats stats_;///< Timing statistics.
  Scheduler::TimedTask<ProgramManager> timer_task_; ///< Timer to check programs.
};
