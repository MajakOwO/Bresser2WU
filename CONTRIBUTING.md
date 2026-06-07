# Contributing to Bresser Weather Sensor to WU + APRS-IS Gateway

Thank you for considering a contribution to this project! Here are some guidelines to help you get started.

## Code of Conduct

This project adheres to the Contributor Code of Conduct. By participating, you are expected to uphold this code. Please report unacceptable behavior to the project maintainers.

## How to Contribute

### Reporting Bugs

Before creating a bug report, please check the issue list as you might find out that you don't need to create one. When you are creating a bug report, please include as many details as possible:

- **Use a clear, descriptive title**
- **Describe the exact steps which reproduce the problem**
- **Provide specific examples to demonstrate the steps**
- **Describe the behavior you observed after following the steps**
- **Explain which behavior you expected to see instead and why**
- **Include serial monitor output** (use code blocks with ```` ``` ````)
- **Include your hardware setup** (ESP32 variant, BMP280 presence, etc.)
- **Include your configuration** (without sensitive credentials)

### Suggesting Enhancements

Enhancement suggestions are tracked as GitHub issues. When creating an enhancement suggestion, please include:

- **Use a clear, descriptive title**
- **Provide a step-by-step description of the suggested enhancement**
- **Provide specific examples to demonstrate the steps**
- **Describe the current behavior** and **the expected behavior**
- **Explain why this enhancement would be useful**

## Development Setup

### Prerequisites
- Visual Studio Code
- PlatformIO IDE extension
- ESP32 development board
- Bresser 7-in-1 weather sensor

### Building Locally

1. **Clone the repository**
   ```bash
   git clone https://github.com/MajakOwO/Bresser2WU.git
   cd Bresser2WU
   ```

2. **Open in VS Code with PlatformIO**
   ```bash
   code .
   ```

3. **Build the project**
   - Click PlatformIO icon in sidebar
   - Select your environment
   - Click "Build" or press `Ctrl+Alt+B`

4. **Upload to device**
   - Connect ESP32 via USB
   - Click "Upload" in PlatformIO or press `Ctrl+Alt+U`

5. **Monitor output**
   - Click "Serial Monitor" in PlatformIO or press `Ctrl+Alt+S`
   - Baud rate: 115200

## Pull Request Process

1. **Fork the repository** and create a branch
   ```bash
   git checkout -b feature/your-feature-name
   ```

2. **Make your changes**
   - Keep commits logical and atomic
   - Use clear, descriptive commit messages
   - Follow the existing code style

3. **Test thoroughly**
   - Test on actual hardware when possible
   - Check Serial output for errors
   - Verify both WU and APRS functionality if applicable

4. **Update documentation**
   - Update README.md if behavior changes
   - Add comments to complex code sections
   - Update this file if process changes

5. **Push and create Pull Request**
   ```bash
   git push origin feature/your-feature-name
   ```
   - Provide clear description of changes
   - Link related issues: `Fixes #123`
   - Include serial output showing the feature works

## Code Style Guidelines

### C++ Coding Standards

- **Indentation**: 4 spaces (no tabs)
- **Line length**: Maximum 100 characters
- **Naming conventions**:
  - Functions: `camelCase()`
  - Classes: `PascalCase`
  - Constants: `UPPER_CASE`
  - Member variables: `memberVariable`
  - Private members: `_memberVariable` or prefix with `priv`

### Comments

- Add file headers to new files:
  ```cpp
  ///////////////////////////////////////////////////////////////////////////////////////////////////
  // filename.h
  //
  // Brief description
  //
  // MIT License
  ///////////////////////////////////////////////////////////////////////////////////////////////////
  ```

- Document complex functions:
  ```cpp
  /// Calculate dew point using Magnus formula
  /// @param tempC - Temperature in Celsius
  /// @param humidity - Relative humidity (0-100)
  /// @return Dew point in Celsius
  float calculateDewPoint(float tempC, float humidity) {
      // ...
  }
  ```

### Headers and Includes

- Use header guards: `#ifndef FILENAME_H`
- Group includes: stdlib, project headers, then external libraries
- Avoid circular dependencies

## Testing

### Serial Monitoring

Monitor the device for:
- WiFiManager startup sequence
- Sensor data reception
- APRS connection attempts and messages
- Weather Underground HTTP responses
- Error conditions

### Verification Checklist

Before submitting:
- [ ] Code compiles without warnings
- [ ] No memory leaks or stack overflow
- [ ] APRS packets appear on aprs.fi
- [ ] Weather Underground shows updates
- [ ] WiFiManager configuration persists after restart
- [ ] Rain gauge calculations are accurate
- [ ] Battery/RSSI indicators display correctly

## Documentation

### Updating README

If your changes affect user-visible behavior, update the README:
- Feature descriptions
- Configuration parameters
- Hardware requirements
- Troubleshooting steps

### Adding Examples

If adding new functionality, consider adding an example or documentation:
- Document the feature in README
- Add inline code comments
- Include serial output examples

## Recognition

Contributors will be recognized in:
- Git commit history
- Pull request credits
- Project documentation (with permission)

## Questions?

Feel free to open an issue for clarification or discussion before starting work on major changes.

---

Thank you for contributing! Your improvements help make this project better for everyone. 🙏
