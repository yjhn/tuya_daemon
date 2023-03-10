module("luci.controller.tuya_controller", package.seeall)

function index()
  entry({"admin", "services", "tuya_daemon"}, cbi("tuya_model"), "Tuya IoT", 100)
end
