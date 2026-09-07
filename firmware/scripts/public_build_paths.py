"""Keep developer-machine paths out of public firmware strings and debug data."""
from pathlib import Path

Import("env")

replacements = {
    env.subst("$PROJECT_DIR"): "touch-card/firmware",
    str(Path(env.PioPlatform().get_package_dir("framework-arduinoespressif32"))): "arduino-esp32",
}
env.Append(CCFLAGS=[f"-ffile-prefix-map={old}={new}" for old, new in replacements.items()])
