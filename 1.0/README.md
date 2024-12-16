# GOS2022
This repository contains a released version of GOS2022

# Release information
Version: 1.0
Date: 2024-12-17

# Changes
First release, contains the following OS components and services:

+ driver skeletons (used by the OS)
+ kernel (scheduler, definitions, task handling, OS configuration)
+ services (OS services provided to the user)
	+ error (OS and user-level warning and or fatal error)
	+ gcp (General Communication Protocol)
	+ message (inter-task data exchange)
	+ mutex (task synchronization, resource protection)
	+ queue (data-flow, inter-task data exchange)
	+ shell (basic text-based command interpreter)
	+ signal (event signalling, task synchronization)
	+ sysmon (system monitoring, statistics)
	+ time (basic time definitions, operations, elapsed time handling)
	+ trace (formatted message printing on a given output for debugging)
	+ trigger (lightweight task synchronization without data exchange)

# Remarks
Contribution to this project is encouraged and appriciated.
If you have any questions or ideas, feel free to contact me: gaborr97@gmail.com

(c) 2024, Ahmed Gazar