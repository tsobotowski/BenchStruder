BenchStruder

This is a prototype for a low-cost benchtop 3d printer extruder that enables users to extrude high quality 3D printer filament from failed prints and/or virgin plastic.

Goals:
- Achive sufficient filabment consistancy and reliability using off the shelf wood auger bits found at any hardware store
- Enable safe and reliable at-home estrusion through heater runaway protection and failsafes
- Implement a fully integrated, intuitive GUI
- Implement closed loop control for filament winding
- Achive a total materials cost under $700

# Firmware
Libraries:
-  LiquidCrystal
-  NEXNTC

Buttons:
- The user interface is controlled by two buttons, up/heat and down/extrude
- Button inputs are either a press or hold
- A press occurs for less than a second
- A hold occurs for longer than a second
- Presses are used to set tempreture and disabling
- Holds are used for enabling

Temperature control: 
- The current hardware configuration employs a mechanical relay to control the heating element
- Bang-Bang control is currently implemented
- Switching to PID control through a solid-state relay will massivly improve control accuracy and reliability

States: This firmware implements a state machine architecture
- Idle: The heater and motor are off and the user can use the up and down buttons to set extrusion temperature
- Heating: The heater is being activly controlled to maintain the set temperature
- Extruding: The heater is maintaining the temperature and the extruder is operating NOT IMPLEMENTED
- init_err: The user is locked out of the machine, an initial temperature reading suggests an issue with the sensor
- runaway_err: The user is locked out of the machine, the heater has been powered for a set amount of time without an increase in temperature 

