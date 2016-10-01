# argparse
A humble and simple argument parser written in C++.

`arg::argparse` is designed to handle _simple argument parsing_ (flags and options) and is capable of automatically generating usage instructions.  It is not POSIX compliant. All it tries to do is abstract away boring argument parsing as you would have in a small program and look good while doing so.

## Example

### Definition

``` c++
arg::argparse a = arg::argparse(argv[0])
    .option("--help",   "-h", arg::opt_none,   "show this text")
    .option("--param",  "-p", arg::opt_int,    "set some param")
    .option("--value",  "-v", arg::opt_int,    "set a value", true)
    .option("--string", "-s", arg::opt_string, "set a string", true)
    .option("--float",  "-f", arg::opt_float,  "set a float")
    .flag("--quiet",    "-q",                  "shut the fuck up")
    .parse(argc, argv);
```

The previous snippet defines 2 flags (`--help` and `--quiet` with their respective short forms) and 4 options. The last optional boolean parameter designates an argument as required. A flag can be defined by either passing `arg::opt_none` or using `flag()` instead of `option()`when defining. Pass an empty string when you don't need a short form.

Please note that the actual contents of the strings are not checked in any form, you are not bound to use the same format.

### Printing usage

```c++
if(!a.is_valid() || a.has_flag("--help")) {
    std::cout << a.usage();
    std::exit(-1);
}
```

`is_valid()` only returns true iff all required arguments were encountered and and all arguments were successfully parsed. Example output:

```
Usage: ./a.out [OPTIONS]

Possible options:
	--quiet, -q            shut the fuck up
	--param, -p            set some param
	--help, -h             show this text
	--value, -v            set a value (REQUIRED)
	--float, -f            set a float
	--string, -s           set a string (REQUIRED)
```

The order of options is undefined (options are stored in a hashmap internally).

### Retrieving values

argparse uses `std::experimental::optional<T>` (optional is out of experimental with C++17) under the hood.  A value must be retrieved by long form. (This could be changed easily, but is it necessary?)

Supported data types as of now are:

* `int`
* `char const*`
* `std::string`
* `double`
* `bool`

```c++
int p = a.get<int>("--param").value_or(666);
```

When `--param` is not set, 666 will be substituted as default value. Check out [`std::optional`'s interface](http://en.cppreference.com/w/cpp/utility/optional) if you like.

```c++
int val = a.get<int>("--value").value();
std::string s = a.get<std::string>("--string").value();
```

Since both  `--value` and `--string` are set as required, they are guaranteed to hold a value iff `is_valid()` returned true previously.

```c++
if(a.has_flag("--quiet")) { std::cout << "i'll be quiet\n"; }
if(a.get<bool>("--quiet").value()) { std::cout << "shhhhh\n"; }
```

Two variants are possible. A flag will always hold a value, but is embedded in an optional anyway, for consistency's sake. An optional will coerce into a boolean true if it holds a value, so the following:

```c++
if(a.get<bool>("--quiet")) { std::cout << "psssst\n"; } // not what you might expect
```

... will always return true!

### Checking if a value has been set w/o retrieving

argparse has `has_key()`, `has_opt()` and `has_flag()`. All of them accept a variable amount of string arguments (again, in long form only) and returns true if one of them was passed (→ disjunctive).

```c++
if(a.has_key("--help", "--quiet")) {
    std::cout << "i should be either quiet or helpful or both\n";
}

if(a.has_flag("--help", "--quiet")) {
    // same thing as above, but checks only flags
}

if(a.has_opt("--float", "--string")) {
    // checks only options
}
```
