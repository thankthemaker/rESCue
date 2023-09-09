Import("env")
#print("ENV: ", env.Dump())
env.Replace(PROGNAME="%s" % env.GetProjectOption("custom_firmware_name"))