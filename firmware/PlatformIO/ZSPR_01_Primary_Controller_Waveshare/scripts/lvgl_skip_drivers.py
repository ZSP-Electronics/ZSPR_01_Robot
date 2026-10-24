Import("env")

# LVGL's optional display/input drivers are not used here (RGBPanel is the flush target).
# On Windows their long "../" include chains also exceed MAX_PATH under this project's deep path.
env.AddBuildMiddleware(lambda node: None, "*/lvgl/src/drivers/*")
