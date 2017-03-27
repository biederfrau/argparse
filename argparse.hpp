#include <cassert>
#include <experimental/optional>
#include <iomanip>
#include <string>
#include <sstream>
#include <unordered_set>
#include <unordered_map>
#include <vector>

struct non_homogenous_type {};

template <class... Ts>
struct homogenous_type;

template <class T>
struct homogenous_type<T> {
    using type = T;
    static constexpr bool is_homogenous = true;
};

template <class T, class... Ts>
struct homogenous_type<T, Ts...> {
    using type_of_remaining_parameters = typename homogenous_type<Ts...>::type;
    static constexpr bool is_homogenous = std::is_same<T, type_of_remaining_parameters>::value;
    using type = typename std::conditional<is_homogenous, T, non_homogenous_type>::type;
};

template <class... Ts>
struct is_homogenous_pack {
    static constexpr bool value = homogenous_type<Ts...>::is_homogenous;
};

namespace arg {
    struct unspecialized: std::exception {
        char const* issue;
        unspecialized(char const* i) : issue{ i } {}
        char const* what() const noexcept { return issue; }
    };

    namespace exp = std::experimental;

    // tag
    enum parameter_t {
        opt_int,
        opt_float,
        opt_string,
        opt_none
    };

    // tagged union
    struct args_t {
        parameter_t param_type;
        union {
            double f;
            int i;
            char const* s;
        };
        operator int() const { return i; }
        operator double() const { return f; }
        operator char const*() const { return s; }
        operator std::string() const { return s; }
    };

    class argparse {
    private:
        std::string program_name;
        std::unordered_map<std::string, parameter_t> defined_set;
        std::unordered_map<std::string, std::string> short_map;
        std::unordered_map<std::string, std::string> long_map;
        std::unordered_set<std::string> required_set;
        std::unordered_map<std::string, std::string> help;

        std::unordered_set<std::string> flag_set;
        std::unordered_map<std::string, exp::optional<args_t>> options;
        std::vector<std::string> ignored_options;
        bool valid = false;

        bool is_defined(std::string const& key) const { return defined_set.count(key); }
        bool is_short(std::string const& key) const { return short_map.count(key); }
        bool is_required(std::string const& key) const { return required_set.count(key); }

        void store_flag(std::string const& key) { flag_set.insert(key); }
        void store_int(std::string const& key, std::string const& val) {
            args_t a;
            a.param_type = opt_int;
            a.i = std::stoi(val);
            options[key] = { a };
        }

        void store_float(std::string const& key, std::string const& val) {
            args_t a;
            a.param_type = opt_float;
            a.f = std::stod(val);
            options[key] = { a };
        }

        void store_string(std::string const& key, char const* val) {
            args_t a;
            a.param_type = opt_string;
            a.s = val;
            options[key] = { a };
        }
    public:
        argparse(char const* program_name = "") : program_name{ program_name } {}

        argparse& option(std::string const& id, std::string const& short_id,
                parameter_t param_type, std::string const& helpstr = "",
                bool required = false) {
            defined_set[id] = param_type;
            help[id] = helpstr;

            if(!short_id.empty()) {
                short_map[short_id] = id;
                long_map[id] = short_id;
            }

            if(required) { required_set.insert(id); }

            return *this;
        }

        argparse& flag(std::string const& id, std::string const& short_id,
                std::string const& helpstr = "") {
            return option(id, short_id, opt_none, helpstr);
        }

        bool is_valid() const { return valid; }
        std::vector<std::string> const& ignored_opts() const { return ignored_options; }

        std::string usage() const {
            std::ostringstream ss;
            ss << "Usage: " << program_name << " [OPTIONS]\n\n";
            ss << "Possible options:\n";

            size_t max_width = 0;
            for(auto const& opt: defined_set) {
                if(opt.first.size() > max_width) { max_width = opt.first.size(); }
            }

            max_width += 15;

            for(auto const& opt: defined_set) {
                size_t len = max_width - opt.first.size();
                ss << '\t' << opt.first;
                if(long_map.count(opt.first)) {
                    ss << ", " << long_map.at(opt.first);
                    len -= 4;
                }

                for(size_t i = 0; i < len; ++i) { ss << ' '; }
                ss << help.at(opt.first);

                if(is_required(opt.first)) { ss << " (REQUIRED)"; }
                ss << '\n';
            }

            return ss.str();
        }

        argparse& parse(int argc, char** argv) {
            if(program_name.empty()) { program_name = argv[0]; }

            try {
                for(int i = 1; i < argc; ++i) {
                    std::string option = argv[i];

                    if(is_defined(option) || is_short(option)) {
                        if(is_short(option)) { option = short_map[option]; }

                        switch(defined_set[option]) {
                            case parameter_t::opt_none:
                                store_flag(option);
                                break;
                            case parameter_t::opt_int:
                                if(i + 1 < argc) { store_int(option, argv[++i]); }
                                break;
                            case parameter_t::opt_float:
                                if(i + 1 < argc) { store_float(option, argv[++i]); }
                                break;
                            case parameter_t::opt_string:
                                if(i + 1 < argc) { store_string(option, argv[++i]); }
                                break;
                            default:
                                assert(false && "this should never happen");
                        }
                    } else { ignored_options.push_back(argv[i]); }
                }
            } catch(std::invalid_argument const& e) {
                valid = false;
                return *this;
            }

            valid = true;
            for(auto const& key: required_set) {
                if(!options[key] && !options[short_map[key]]) { valid = false; break; }
            }
            return *this;
        }

        template <class... Args>
        bool has_key(Args... keys) const {
            static_assert(is_homogenous_pack<Args...>::value,
                          "\033[1;31merror\033[0m: nonhomogenous keys");
            for(auto const& k: { ::std::forward<Args>(keys)... }) {
                if(options.count(k) || flag_set.count(k)) { return true; }
            }
            return false;
        }

        template <class... Args>
        bool has_flag(Args... flags) const {
            static_assert(is_homogenous_pack<Args...>::value,
                          "\033[1;31merror\033[0m: nonhomogenous keys");
            for(auto const& k: { ::std::forward<Args>(flags)... }) {
                if(flag_set.count(k)) { return true; }
            }
            return false;
        }

        template <class... Args>
        bool has_opt(Args... opts) const {
            static_assert(is_homogenous_pack<Args...>::value,
                          "\033[1;31merror\033[0m: nonhomogenous keys");
            for(auto const& k: { ::std::forward<Args>(opts)... }) {
                if(options.count(k)) { return true; }
            }
            return false;
        }

        template <class T>
        exp::optional<T> get(std::string const&) const {
            throw unspecialized("i can't take this shit any longer!!!11");
        }
    };

    template <>
    exp::optional<int> argparse::get<int>(std::string const& key) const {
        if(options.count(key)) { return { int(options.at(key).value()) }; }
        return { };
    }

    template <>
    exp::optional<char const*> argparse::get<char const*>(std::string const& key) const {
        if(options.count(key)) { return { options.at(key).value().s }; }
        return { };
    }

    template <>
    exp::optional<std::string> argparse::get<std::string>(std::string const& key) const {
        if(options.count(key)) { return { options.at(key).value().s }; }
        return { };
    }

    template <>
    exp::optional<double> argparse::get<double>(std::string const& key) const {
        if(options.count(key)) { return { double(options.at(key).value()) }; }
        return { };
    }

    // yeah i know optional is hella unnecessary here but for the sake of
    // consistency...
    template <>
    exp::optional<bool> argparse::get<bool>(std::string const& key) const {
        if(options.count(key) || flag_set.count(key)) { return { true }; }
        return { false };
    }
}
