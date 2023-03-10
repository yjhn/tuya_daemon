map = Map("tuya")

section = map:section(NamedSection, "tuya_daemon", "daemon", "Tuya daemon")

device_id = section:option(Value, "device_id", "Tuya device ID")
device_secret = section:option(Value, "device_secret", "Tuya device secret")
product_id = section:option(Value, "product_id", "Tuya product ID")

autostart = section:option(Flag, "autostart", "Start automatically")
autostart.default = "0"
autostart.rmempty = false

become_daemon = section:option(Flag, "become_daemon", "Become a daemon")
become_daemon.default = "0"
become_daemon.rmempty = false

log_level = section:option(ListValue, "log_level", "Log level", "Maximum level for which messages will be logged. Values are the as defined in POSIX syslog.")
log_level:value("0", "emergency")
log_level:value("1", "alert")
log_level:value("2", "critical")
log_level:value("3", "error")
log_level:value("4", "warning")
log_level:value("5", "notice")
log_level:value("6", "info")
log_level:value("7", "debug")
log_level.default = "4"

return map
