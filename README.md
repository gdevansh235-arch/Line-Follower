<img width="982" height="808" alt="image" src="https://github.com/user-attachments/assets/fbcdf7f2-e76f-4b7c-8de4-b1fe95324b20" />

# Line-Follower Robot

A line-following robot that uses infrared (IR) sensors to detect a line on the floor and adjusts its motors to follow the path.

## Project goals

- Research different robot designs and choose a practical line-follower design.
- Model the chassis and plan the placement of components in Onshape.
- Build the chassis and component mounts.
- Develop and test the robot firmware.
- Document the design and build process in this repository.

## How it works

1. IR sensors detect the contrast between the track and its surroundings.
2. The controller reads the sensor outputs.
3. The firmware adjusts the left and right motors to steer the robot toward the line.

## Planned hardware

The exact components and pin assignments should be confirmed against the parts used in the physical build. The example firmware currently assumes an Arduino-compatible board, two digital line sensors, and an L298N-style dual motor driver; update the pin definitions and sensor logic if your hardware differs.

- Controller: Arduino-compatible board (confirm exact model)
- Sensors: two digital IR line sensors (left and right)
- Motor driver: L298N-compatible dual H-bridge
- Drive: two DC geared motors and wheels
- Chassis: designed in Onshape

## Repository structure

- `firmware/line_follower.ino` — starter Arduino firmware
- `JOURNAL.md` — project journal for Days 1–4

## Current progress

- **Day 1:** Researched different line-follower robot designs.
- **Day 2:** Finalised the line-follower concept and learned how it works.
- **Day 3:** Created an Onshape account, started modelling the chassis, and planned component placement.
- **Day 4:** Worked on building/modeling the components.

## Next steps

1. Confirm the controller, sensor model, motor driver, and motor wiring.
2. Complete the chassis and verify that all components fit.
3. Upload and test the firmware on the actual hardware.
4. Add photos, CAD screenshots, wiring details, and test results as the build progresses.

> **Note:** The firmware is a starter template, not a verified final program. Test the motor direction and sensor output with the wheels lifted before running the robot on the floor.
