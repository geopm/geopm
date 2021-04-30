#include <exception>
#include <iostream>
#include <sstream>
#include <cstdlib>
#include <cerrno>
#include <vector>
#include <string>
#include <systemd/sd-bus.h>


void sdbus_check(const std::string &function, int err, const sd_bus_error *bus_error)
{
    if (err < 0) {
        std::ostringstream error_message;
        error_message << "Failed to call sd-bus function \"" << function << "\": "
                      << "errno: " << errno;
        if (bus_error != nullptr) {
            error_message << bus_error->name << ": "
                          << bus_error->message << "\n";
        }
        throw std::runtime_error(error_message.str());
    }
}

int main(int argc, char **argv)
{
    sd_bus_error bus_error = SD_BUS_ERROR_NULL;
    sd_bus_message *bus_message = NULL;
    sd_bus *bus = NULL;
    int err = 0;

    sdbus_check("sd_bus_open_system",
                sd_bus_open_system(&bus),
                nullptr);

    sdbus_check("sd_bus_call_method",
                sd_bus_call_method(bus, "io.github.geopm",
                                   "/io/github/geopm",
                                   "io.github.geopm",
                                   "PlatformGetAllAccess",
                                   &bus_error, &bus_message, ""),
                &bus_error);
    sd_bus_close(bus);

    sdbus_check("sd_bus_message_enter_container",
                sd_bus_message_enter_container(bus_message, SD_BUS_TYPE_STRUCT, "asas"),
                nullptr);
    sdbus_check("sd_bus_message_enter_container",
                sd_bus_message_enter_container(bus_message, SD_BUS_TYPE_ARRAY, "s"),
                nullptr);

    std::vector<std::string> signal_names;
    for (bool is_done = false; !is_done;) {
        const char *c_str = nullptr;
        int err = sd_bus_message_read(bus_message, "s", &c_str);
        sdbus_check("sd_bus_message_read", err, nullptr);
        if (err == 0) {
            is_done = true;
        }
        else {
            signal_names.push_back(c_str);
        }
    }
    sdbus_check("sd_bus_message_exit_container",
                sd_bus_message_exit_container(bus_message),
                nullptr);


    sdbus_check("sd_bus_message_enter_container",
                sd_bus_message_enter_container(bus_message, SD_BUS_TYPE_ARRAY, "s"),
                nullptr);

    std::vector<std::string> control_names;
    for (bool is_done = false; !is_done;) {
        const char *c_str = nullptr;
        int err = sd_bus_message_read(bus_message, "s", &c_str);
        sdbus_check("sd_bus_message_read", err, nullptr);
        if (err == 0) {
            is_done = true;
        }
        else {
            control_names.push_back(c_str);
        }
    }
    sdbus_check("sd_bus_message_exit_container",
                sd_bus_message_exit_container(bus_message),
                nullptr);

    sdbus_check("sd_bus_message_exit_container",
                sd_bus_message_exit_container(bus_message),
                nullptr);


    std::cout << "SIGNALS" << "\n";
    std::cout << "-------" << "\n";
    for (const auto &name : signal_names) {
        std::cout << name << "\n";
    }
    std::cout << "\nCONTROLS" << "\n";
    std::cout << "--------" << "\n";
    for (const auto &name : control_names) {
        std::cout << name << "\n";
    }

}
