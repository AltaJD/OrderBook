#include <iostream>
#include <string>
#include <sstream>
#include <fstream>
#include <vector>
#include <chrono>
#include <variant>
#include "order_book.hpp"

struct comma_delimiter : std::ctype<char> {
    comma_delimiter() : std::ctype<char>(get_table()) {}

    static std::ctype_base::mask const* get_table() {
        static std::vector<std::ctype_base::mask> 
            rc(std::ctype<char>::table_size, std::ctype_base::mask());

        rc[','] = std::ctype_base::space;
        rc['\n'] = std::ctype_base::space;
        rc['\r'] = std::ctype_base::space;
        return &rc[0];
    }
};

class DataParser
{ 
    private:
        std::string file_path;
        int orders_num_limit;
        OrderTable orders_table;
        void process_order_details(const std::string& symbol,
                                    double bid_p,
                                    double ask_p,
                                    double trade_p,
                                    unsigned int bid_v,
                                    unsigned int ask_v,
                                    unsigned int trade_v,
                                    short int update_type,
                                    const std::string& date,
                                    double seconds,
                                    const std::string& condition)
        {
            UpdateType type = process_type(update_type);
            auto date_details = parse_date(date);
            auto time_point = createTimePoint(date_details[0], date_details[1], date_details[2], seconds);
            Order order(symbol, bid_p, ask_p, trade_p, bid_v, ask_v, trade_v, condition, type, date, time_point);
            orders_table.processOrder(order);
        }
    public:
        DataParser(const std::string& path): file_path{path}, orders_table()
        {
            std::cout<<"Data Parser has been initiated"<<std::endl;
        }
        DataParser(const std::string& path, int data_num): file_path{path}, orders_num_limit{data_num} {}
        ~DataParser() {}
        void start()
        {
            std::cout<<"Started reading file"<<std::endl;
            std::ifstream classFile(file_path);
            std::string line;

            while(std::getline(classFile, line, '\n'))
            {
                parse_line(line);
            }

            std::cout<<"Reading file has been finished"<<std::endl;
        }
        void test_start()
        {
            std::cout<<"Started reading file"<<std::endl;
            std::ifstream classFile(file_path);
            std::string line;
            int counter = 0;

            while(std::getline(classFile, line, '\n'))
            {
                parse_line(line);
                counter++;
                if (counter == orders_num_limit || orders_num_limit == 0)
                {
                    break;
                }
            }

            std::cout<<"Reading file has been finished"<<std::endl;
        }
        void parse_line(const std::string& line)
        {
            /* Each data entity has certain position on the line according to the commas
            The function assign the variables with the data entity value according to its position.
            For skipping unnecessory data entity, "skip" variable has been allocated.
            */
            std::istringstream iss(line);
            iss.imbue(std::locale(std::locale(), new comma_delimiter)); // append the delimiter of 'iss' with comma (overwriting the previous object)
            
            std::string skip; // overwrite the unnessory order details
            std::string symbol;
            double bid_price;
            double ask_price;
            double trade_price;
            unsigned int bid_volume;
            unsigned int ask_volume;
            unsigned int trade_volume;
            short int update_type; // can be withing range of [1, 3]
            std::string date;
            double seconds;
            std::string condition_codes;

            // assign the value of variables according to the position of data entity in the line
            iss >> symbol >> skip >> bid_price >> ask_price >> trade_price >> bid_volume >> ask_volume 
            >> trade_volume >> update_type >> skip >> date >> seconds >> skip >> skip
            >> condition_codes;

            if (valid_order(condition_codes))
            {
                condition_codes == "@1" ? condition_codes = "": condition_codes=condition_codes;
                process_order_details(symbol,
                                    bid_price,
                                    ask_price,
                                    trade_price,
                                    bid_volume,
                                    ask_volume,
                                    trade_volume,
                                    update_type,
                                    date,
                                    seconds,
                                    condition_codes);
            }
        }
        bool valid_order(const std::string& condition_code)
        {
            /* The function provide the validity of the order.
            Based on the passed arguments and formulas mentioned in the function,
            it is easier to modify the conditions of validity.
            If condition code is @1 it indicates that 15th column is missing => condition_code is "" (empty string)
            */
            bool validity = (containsSubstring(condition_code, "XT") || condition_code == "@1");
            return validity;
        }
        void save_orders(const std::string& destination_file) const
        {
            orders_table.save(destination_file);
        }
        void show_summary()
        {
            std::cout<<"Data Parser Details:"<<std::endl;
            std::cout<<"Data extracted from: ("<<file_path<<")"<<std::endl;
            orders_table.show_summary();
        }
        static std::vector<int> parse_date(const std::string& date)
        {
            /* The date in string is represented as  20150420, which states: 
            Year: 2015
            Month: 04
            Day: 2
            :param data is a string "20150420"
            :returns [2015, 4, 2]
            */
            std::vector<int> result;
            int year = std::stoi(date.substr(0, 4));
            int month = std::stoi(date.substr(4, 2));
            int day = std::stoi(date.substr(6, 2));
            result.push_back(year);
            result.push_back(month);
            result.push_back(day);

            return result;
        }

        static std::chrono::system_clock::time_point createTimePoint(int year, int month, int day, double seconds)
        {
            /* The date is converted to chrono time point based on:
            year, month, day and seconds, where
            seconds counted past midnight
            :returns time_point in chrono
            */
            std::chrono::system_clock::time_point timePoint = std::chrono::system_clock::from_time_t(0);
            timePoint += std::chrono::hours(24 * (day - 1));
            timePoint += std::chrono::hours(24 * 30 * (month - 1));
            timePoint += std::chrono::hours(24 * 365 * (year - 1970));
            timePoint += std::chrono::seconds(static_cast<long long>(seconds));

            return timePoint;
        }

        static UpdateType process_type(short int update_type)
        {
            UpdateType type;
            switch (update_type)
            {
                case 1:
                    return UpdateType::Trade;
                    break;
                case 2:
                    return UpdateType::ChangeToBid;
                    break;
                case 3:
                    return UpdateType::ChangeToAsk;
                    break;
                default:
                    break;
            }
            return type;
        }

        static bool containsSubstring(const std::string& str, const std::string& substr) {
            return str.find(substr) != std::string::npos;
        }

};