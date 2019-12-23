# Chrome-EC-Tool

Date : 2019/12/23
1. Sync with master branch
2. Delete get_uptime_info command

Date : 2019/12/14
1. Sync with master branch

Date : 2019/12/13
1. Add get_uptime_info command
   a. display EC reset cause
   b. display the time since EC boot 
2. Modify batterymonitor command
   a. auto find bat/chg i2c port, and it can also be specifed
   b. automatically print recorrd item in the log file

Date : 2019/11/27
1. Automatic searching battery/charger i2c port

Date : 2019/11/25
1. Modify ectool for display and log battery information
   usage : ectool batterymonitor [i2c_port]

Date : 2019/11/21  
1.  Add battery info display and log function
    usage : ectool batterymonitor
