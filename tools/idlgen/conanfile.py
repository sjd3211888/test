from conans import ConanFile, CMake, tools
import semver
import os


def get_version():
    git = tools.Git()
    try:
        # If the last commit is not a tag, git.get_tag() returns None
        if git.get_tag() != None:
            return git.get_tag()
        else:
            fo = open("VERSION", "r")
            line = fo.readline()
            print("VERSION: %s" % (line))
            if line != "":
                return line
            else:
                return ("%s_%s" % (git.get_branch(), git.get_revision()[0:7])).replace(' ', '').replace('(', '').replace(')', '')
    except:
        return ""


class LiDDSDDSConan(ConanFile):
    name = "lidds-gen"
    license = "Apache License 2.0"
    url = "gitlab.chehejia.com/LiDDS/LiDDS-Tools/LiDDS-IDL-Gen.git"
    branch = "master"
    description = "Conan wrapper for LiDDS-DDS-GEN"
    settings = "os"
    options = {"Build_Java": [True, False]}
    # , "RPCProto" : "rpcdds"}
    default_options = {"Build_Java": False}
    generators = "cmake"

    # exports_sources = "CMakeLists.txt", "cmake/*"

    def init(self):
        self.version = "1.1.0-rc1"

    def source(self):
        # git = tools.Git()
        # git.clone("git@gitlab.chehejia.com:LiDDS/LiDDS-Tools/LiDDS-IDL-Gen.git", "master")#%self.version)
        self.run("git clone https://GITLAB_ACCESS_TOKEN:%s@%s . && git checkout %s" %
                 (tools.get_env("GITLAB_ACCESS_TOKEN"), self.url, self.branch))

    # def build_requirements(self):
    #     self.build_requires("zulu-openjdk/11.0.8")

    def _configure_cmake(self):
        cmake = CMake(self)
        cmake.verbose = True

        def add_cmake_option(option, value):
            var_name = "{}".format(option).upper()
            value_str = "{}".format(value)
            var_value = "ON" if value_str == 'True' else "OFF" if value_str == 'False' else value_str
            cmake.definitions[var_name] = var_value

        for option, value in self.options.items():
            add_cmake_option(option, value)

        cmake.configure()
        return cmake

    def build(self):
        #cmake = self._configure_cmake()
        # cmake.build()
        # scripts_vbsddsgen = os.path.join(self.build_folder, "scripts", "vbsddsgen")
        # tools.replace_in_file(
        #         scripts_vbsddsgen,
        #         "java -version &>/dev/null",
        #         "java -version"
        #     )
        # self.run("./gradlew assemble")
        pass

    def package(self):
        #cmake = self._configure_cmake()
        # cmake.install()

        self.copy(src="share/idlgen/java", pattern="*.jar",
                  dst="share/idlgen/java", keep_path=False)
        self.copy(src="", pattern="LICENSE", dst="share/", keep_path=False)
        self.copy(src="scripts", pattern="*", dst="bin", keep_path=False)
        pass

    def package_info(self):
        self.deps_user_info
        #self.cpp_info.libs = tools.collect_libs(self)
        pass
