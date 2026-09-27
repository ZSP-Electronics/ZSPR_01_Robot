Import("env")


def erase_before_upload(source, target, env):
    env.AutodetectUploadPort()
    env.Execute("$ERASECMD")


env.AddPreAction("upload", erase_before_upload)
