## 1.1 (2025-07-29)
Changes:
  - gos_driver.h: System monitoring drivers removed.
  - gos_shell_driver.h: Doxygen comments updated.
  - gos_sysmon_driver.h: Removed (sysmon is now part of library).
  - gos_timer_driver.h: Doxygen comments updated.
  - gos_trace_driver.h: Doxygen comments updated.
  - gos_driver.c: Initializer modified due to removed sysmon driver.
  - gos_sysmon_driver.c: Removed (sysmon is now part of library).
  - gos_kernel.h: gos_preResetHook_t added, gos_kernelRegisterPreResetHook() added, GOS_UNUSED_PAR() added, GOS_CONVERT_RESULT() added.
  - gos_kernel.c: gos_kernelRegisterPreResetHook() added.
  - gos_task.c: gos_taskCheckDescriptor check logic inverted, gos_taskGetDataByIndex and gos_taskGetData invalid task ID check added.
  - gos.h: OS version updated, platform and application initializer prototypes added here.
  - gos_error.h: Doxygen comments updated.
  - gos_gcp.h: Doxygen comments updated.
  - gos_message.h: Doxygen comments updated.
  - gos_mutex.h: Doxygen comments updated.
  - gos_queue.h: Doxygen comments updated.
  - gos_shell.h: Doxygen comments updated.
  - gos_signal.h: Doxygen comments updated.
  - gos_sysmon.h: Removed (sysmon is now part of library).
  - gos_time.h: Doxygen comments updated.
  - gos_trace.h: Doxygen comments updated.
  - gos_trigger.h: Doxygen comments updated.
  - gos.c: GOS_CONCAT_RESULT usage added, sleep removed from system task dump handling, sysmon removed, gos_platformDriverInit removed, gos_userApplicationInit removed.
  - gos_error.c: Year in startup logo changed.
  - gos_gcp.c: GOS_CONCAT_RESULT usage added, max. chunk handling added.
  - gos_message.c: Initializer result logic inverted.
  - gos_shell.c: Configurable shell startup delay added.
  - gos_signal.c: GOS_SIGNAL_DAEMON_POLL_TIME_MS added back, initializer result logic inverted, caller task descriptor moved from function to file-level.
  - gos_sysmon.c: Removed (sysmon is now part of library).
  - gos_time.c: Initializer result logic inverted.
  - gos_trace.c: Initializer result logic inverted, GOS_CONCAT_RESULT usage added, trace timestamp length changed to 46.
  - gos_trigger.c: Wait function condition changed from greater or equal to equal only.

## 1.0 (2025-04-07)
First release.
