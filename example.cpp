#include <iostream>
#include "argparse.hpp"

int main(int argc, char** argv) {
    arg::argparse a = arg::argparse(argv[0])
        .option("--help",   "-h", arg::opt_none,   "show this text")
        .option("--param",  "-p", arg::opt_int,    "set some param")
        .option("--value",  "-v", arg::opt_int,    "set a value", true)
        .option("--string", "-s", arg::opt_string, "set a string", true)
        .option("--float",  "-f", arg::opt_float,  "set a float")
        .flag("--quiet",    "-q",                  "shut the fuck up")
        .parse(argc, argv);

    if(!a.is_valid() || a.has_flag("--help")) {
        std::cout << a.usage();
        std::exit(-1);
    }

    // is required, hence guaranteed to hold a value if valid
    int val = a.get<int>("--value").value();
    std::cout << "value: " << val << '\n';
    std::string s = a.get<std::string>("--string").value();
    std::cout << "string: " << s << '\n';

    int p = a.get<int>("--param").value_or(666);
    std::cout << "param: " << p << '\n';

    double f = a.get<double>("--float").value_or(0.333);
    std::cout << "float: " << f << '\n';

    if(a.has_flag("--quiet")) { std::cout << "i'll be quiet\n"; }
    if(a.get<bool>("--quiet").value()) { std::cout << "shhhhh\n"; }
    if(a.get<bool>("--quiet")) { std::cout << "psssst\n"; } // not what you might expect

    if(a.has_key("--help", "--quiet")) {
        std::cout << "i should be either quiet or helpful or both\n";
    }
}
