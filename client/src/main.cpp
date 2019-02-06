#include "./tcp-client/tcp-client.h"

#include <signal.h>
#include <iostream>
#include <boost/program_options.hpp>

boost::program_options::variables_map parse_command_line(int argc, const char* const *argv)
{
    namespace po = boost::program_options;
    po::variables_map vm;
    po::options_description desc{"Options"};

    try {
        desc.add_options()
        ("help,h", "Help message")
        ("client,c", po::value<std::uint32_t>()->required(), "Client ID")
        ("host,h", po::value<std::string>()->default_value("example.com"), "Host to connect")
        ("port,p", po::value<std::uint16_t>()->default_value(8888), "Port to connect")
        ("max_messages,m", po::value<std::uint32_t>()->default_value(1000), "Max messages count to send");
        po::store(po::parse_command_line(argc, argv, desc), vm);
        po::notify(vm);
    } catch (const po::required_option &ex) {
        if(vm.count("help")) {
            std::cout << desc << "\n" << std::endl;
        } else {
            throw ex;
        }
    }
    return vm;
}


int main(int argc, char **argv)
{
    signal(SIGPIPE, SIG_IGN);
    try {
        const auto params{parse_command_line(argc, argv)};
        if(params.count("help")) {
            return 0;
        }
        const std::uint32_t client_id{params["client"].as<std::uint32_t>()};
        const std::string host{params["host"].as<std::string>()};
        const std::uint16_t port{params["port"].as<std::uint16_t>()};
        const std::uint32_t max_msg{params["max_messages"].as<std::uint32_t>()};

        tcp_client::tcp_client client{client_id, host, port, max_msg};
        client.start();
        client.stop();
    } catch (const std::exception &ex) {
        std::cerr << "Client failed with error: " << ex.what() << std::endl;
    } catch (...) {
        std::cerr << "Client failed with unknown error" << std::endl;
    }
    return 0;
}
