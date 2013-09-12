
add_library(log-syslog MODULE main.c)
set_target_properties(log-syslog PROPERTIES OUTPUT_NAME syslog)
INSTALL_MODULE(log-syslog log)
