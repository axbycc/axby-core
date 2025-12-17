#include "app/flag.h"
#include "app/main.h"
#include "debug/log.h"

APP_FLAG(bool, the_bool_flag, false, "an example bool flag");
APP_FLAG(std::string,
         the_string_flag,
         "default value",
         "an example string flag");

int main(int argc, char *argv[]) {
    __APP_MAIN_INIT__;

    APP_UNPACK_FLAG(the_bool_flag);
    APP_UNPACK_FLAG(the_string_flag);
    LOG(INFO) << "Hello world!";
    LOG(INFO) << " The bool flag was " << the_bool_flag;
    LOG(INFO) << " The string flag was " << the_string_flag;
    return 0;
}
