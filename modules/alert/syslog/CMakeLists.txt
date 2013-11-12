
add_library(alert-syslog MODULE main.c)
set_target_properties(alert-syslog PROPERTIES OUTPUT_NAME syslog)
INSTALL_MODULE(alert-syslog alert)
