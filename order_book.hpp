#include <iostream>
#include <iomanip>
#include <string>
#include <vector>
#include <chrono>
#include <map>
#include <algorithm>
#include <stack>

enum class UpdateType
{
    Trade,
    ChangeToBid,
    ChangeToAsk,    
};

class Order
{
    private:
        const std::string symbol;
        double bid_price;
        double ask_price;
        double trade_price;
        unsigned int bid_volume;
        unsigned int ask_volume;
        unsigned int trade_volume;
        const std::string condition_code;
        UpdateType type;
        const std::string date;
        std::chrono::system_clock::time_point time;
    public:
        Order(const std::string& symb,
            double bid_p, 
            double ask_p, 
            double trade_p,
            unsigned int bid_vol,
            unsigned int ask_vol,
            unsigned int trade_vol,
            const std::string& condition_code,
            UpdateType type,
            const std::string date,
            std::chrono::system_clock::time_point timestamp):
            symbol{symb}, 
            bid_price{bid_p},
            ask_price{ask_p},
            trade_price{trade_p},
            bid_volume{bid_vol},
            ask_volume{ask_vol},
            trade_volume{trade_vol},
            condition_code{condition_code},
            type{type},
            date{date},
            time{timestamp}
        {}
        const std::string getSymbol() const { return symbol; }
        double getBidPrice() const { return bid_price; }
        double getAskPrice() const { return ask_price; }
        double getTradePrice() const { return trade_price; }
        double getBidAskSpread() const { return ask_price - bid_price; }
        unsigned int getBidVolume() const { return bid_volume; }
        unsigned int getAskVolume() const { return ask_volume; }
        unsigned int getTradeVolume() const { return trade_volume; }
        std::chrono::system_clock::time_point getTimePoint() const { return time; }
        UpdateType getType() const { return type; }
        ~Order() {}
        void show_summary()
        {
            std::cout<<"Symbol: "<<symbol<<std::endl;
            std::cout<<"Condition Code: "<<condition_code<<std::endl;
        }
};

class OrderBook
{
    /* The Order book represents the list of the order per symbol */
    private:
        const std::string symbol;
        std::vector<Order> orders;
        double mean_time_trades;   // mean time between trades
        double median_time_trades; // medina time between trades
        double longest_time_trades;

        double mean_time_tick;      // mean time between tick changes
        double median_time_tick;    // median time between tick changes
        double longest_time_tick;   // longest time between tick changes

        double mean_spread;         // mean bid ask spread
        double median_spread;       // median bid ask spread

        std::vector<std::chrono::seconds> timeDifferences;
        std::chrono::system_clock::time_point previousTradeTime;

        std::vector<std::chrono::seconds> timeTickDifferences;
        std::stack<std::pair<double, Order>> bidPrices; // store the latest changes in the bid prices with its order
        std::stack<std::pair<double, Order>> askPrices; // store the latest changes in the ask prices with its order

        std::vector<double> spreadList;

        static double get_mean(std::vector<std::chrono::seconds> time_diff)
        {
            // Calculate the sum of time differences
            std::chrono::seconds sumTimeDifferences(0);
            for (const auto& timeDifference : time_diff)
            {
                sumTimeDifferences += timeDifference;
            }

            // Calculate the mean time between "Trade" orders
            if (time_diff.empty())
            {
                return 0.0;
            }
            return static_cast<double>(sumTimeDifferences.count()) / time_diff.size();
        }

        static double get_median(std::vector<std::chrono::seconds> time_diff)
        {
            // Calculate the median time between "Trade" orders
            if (time_diff.empty())
            {
                return 0.0;
            }
            std::sort(time_diff.begin(), time_diff.end()); // sort in place or new variable can be used to keep order

            if (time_diff.size() % 2 == 0)
            {
                // If the number of elements is even, take the average of the two middle elements
                return static_cast<double>(time_diff[time_diff.size() / 2 - 1].count() + time_diff[time_diff.size() / 2].count()) / 2;
            }
            // If the number of elements is odd, take the middle element
            return static_cast<double>(time_diff[time_diff.size() / 2].count());
            
        }

        static double get_largest(std::vector<std::chrono::seconds> time_diff)
        {
            std::sort(time_diff.begin(), time_diff.end());
            if (!time_diff.empty())
            {
                return static_cast<double>(time_diff.back().count());
            }
            else
            {
                return 0.0;
            }
        }

        static double get_mean(const std::vector<double>& spreads)
        {
            if (spreads.empty())
            {
                return 0.0;
            }

            double sum = 0.0;
            for (double spread : spreads)
            {
                sum += spread;
            }

            return sum / spreads.size();
        }

        static double get_median(std::vector<double> spreads)
        {
            if (spreads.empty())
            {
                return 0.0;
            }

            std::sort(spreads.begin(), spreads.end());

            if (spreads.size() % 2 == 0)
            {
                return (spreads[spreads.size() / 2 - 1] + spreads[spreads.size() / 2]) / 2;
            }
            else
            {
                return spreads[spreads.size() / 2];
            }
        }

    public:
        OrderBook() = default;
        OrderBook(const std::string& symbol):symbol{symbol},
                                            mean_time_trades{0.0},
                                            median_time_trades{0.0},
                                            longest_time_trades{0.0},
                                            mean_time_tick{0.0},
                                            median_time_tick{0.0},
                                            longest_time_tick{0.0},
                                            mean_spread{0.0},
                                            median_spread{0.0}
        {}
        ~OrderBook() {}
        void addOrder(Order order)
        {
            orders.push_back(order);
            analyze(order);
        }

        void analyze(Order order)
        {
            /* The function specify the order of statistical analysis over the orders */
            auto timeTradeDifferences = getTimeDifferencesTrade(order);
            mean_time_trades    = get_mean(timeTradeDifferences);
            median_time_trades  = get_median(timeTradeDifferences);
            longest_time_trades = get_largest(timeTradeDifferences);

            auto timeTickDifferences = getTickTimeDifferences(order);
            
            mean_time_tick      = get_mean(timeTickDifferences);
            median_time_tick    = get_median(timeTickDifferences);
            longest_time_tick   = get_largest(timeTickDifferences);

            auto BidAskSpreads = getBidAskSpreadList(order);
            
            mean_spread         = get_mean(BidAskSpreads);
            median_spread       = get_median(BidAskSpreads);
        }

        std::vector<std::chrono::seconds> getTimeDifferencesTrade(Order order)
        {
            /* The function determines the time difference between consecutive "Trade" orders */
            if (order.getType() == UpdateType::Trade)
            {
                if (previousTradeTime != std::chrono::system_clock::time_point())
                {
                    std::chrono::seconds timeDifference = std::chrono::duration_cast<std::chrono::seconds>(order.getTimePoint() - previousTradeTime);
                    timeDifferences.push_back(timeDifference);
                }
                previousTradeTime = order.getTimePoint();
            }
            return timeDifferences;
        }
        
        std::vector<std::chrono::seconds> getTickTimeDifferences(Order order)
        {
            if (order.getType() == UpdateType::ChangeToBid)
            {
                if (!bidPrices.empty())
                {
                    if (order.getBidPrice() == bidPrices.top().first) {return timeTickDifferences;} // no change in the price => no tick
                    std::chrono::seconds timeDiff = std::chrono::duration_cast<std::chrono::seconds>(order.getTimePoint() - bidPrices.top().second.getTimePoint());
                    timeTickDifferences.push_back(timeDiff);
                    // askPrices.pop();
                }

                bidPrices.push(std::make_pair(order.getBidPrice(), order));
            }
            else if (order.getType() == UpdateType::ChangeToAsk)
            {
                if (!askPrices.empty())
                {
                    if (order.getBidPrice() == askPrices.top().first) {return timeTickDifferences;} // no change in the price => no tick
                    std::chrono::seconds timeDiff = std::chrono::duration_cast<std::chrono::seconds>(order.getTimePoint() - askPrices.top().second.getTimePoint());
                    timeTickDifferences.push_back(timeDiff);
                    // bidPrices.pop();
                }
                askPrices.push(std::make_pair(order.getBidPrice(), order));;
            }
            return timeTickDifferences;
        }

        std::vector<double> getBidAskSpreadList(Order order)
        {
            double spread = order.getAskPrice() - order.getBidPrice();
            spreadList.push_back(spread);
            return spreadList;
        }

        int get_orders_num() const
        {
            return orders.size();
        }

        const std::string getSymbol() const 
        {
            return symbol;
        }

        double get_mean_time_trades() const
        {
            return mean_time_trades;
        }

        double get_median_time_trades() const
        {
            return median_time_trades;
        }

        double get_longest_time_trades() const
        {
            return longest_time_trades;
        }

        double get_mean_time_tick() const
        {
            return mean_time_tick;
        }

        double get_median_time_tick() const
        {
            return median_time_tick;
        }

        double get_longest_time_tick() const
        {
            return longest_time_tick;
        }

        double get_mean_spread() const
        {
            return mean_spread;
        }

        double get_median_spread() const
        {
            return median_spread;
        }

        void show_summary() const
        {
            std::cout<<"============================================================================================="<<std::endl;
            std::cout << std::left << std::setw(35) << "Symbol"
                    << std::setw(20) << "Mean Trade Time"
                    << std::setw(20) << "Median Trade Time"
                    << std::setw(20) << "Longest Trade Time"
                    << std::setw(20) << "Mean Tick Time"
                    << std::setw(20) << "Median Tick Time"
                    << std::setw(20) << "Longest Tick Time"
                    << std::setw(20) << "Mean Spread"
                    << std::setw(20) << "Median Spread"
                    << std::endl;

            std::cout << std::left << std::setw(35) << symbol
                    << std::setw(20) << std::fixed << std::setprecision(6) << mean_time_trades
                    << std::setw(20) << median_time_trades
                    << std::setw(20) << longest_time_trades
                    << std::setw(20) << std::fixed << std::setprecision(4) << mean_time_tick
                    << std::setw(20) << median_time_tick
                    << std::setw(20) << longest_time_tick
                    << std::setw(20) << mean_spread
                    << std::setw(20) << median_spread
                    << std::endl;
        }
};

class OrderTable
{
    private:
        std::map<std::string, OrderBook> table;
        void addOrderBook(Order order)
        {
            std::string symbol = order.getSymbol();
            OrderBook book(symbol); // create new book
            book.addOrder(order);
            table.insert(std::make_pair(symbol, book));
        }
    public:
        OrderTable() = default;
        void processOrder(Order order)
        {
            /* Function appends the order to a specific order book based on the symbol 
            As the result, order book will contain only the order with the same symbol type
            */
            if (is_symbol_exists(order.getSymbol()))
            {
                table[order.getSymbol()].addOrder(order);
            }
            else
            {
                addOrderBook(order);
            }
        }

        int getSymbolsNum() { return table.size(); }

        bool is_symbol_exists(const std::string& symbol)
        {
            auto it = table.find(symbol);
            return it != table.end();
        }

        int getTotalOrders()
        {
            int result = 0;
            for(auto& pair: table)
            {
                result += pair.second.get_orders_num();
            }
            return result;
        }

        void show_summary()
        {
            std::cout<<"Order Table Summary"<<std::endl;
            std::cout<<"Number of Symbols in Order Table: "<<getSymbolsNum()<<std::endl;
            for (const auto& pair: table) {
                std::cout<<"\n\tOrder Book ("<<pair.second.get_orders_num()<<")\t"<<std::endl;
                pair.second.show_summary();
            }
            std::cout<<"\nTotal number of Orders: "<<getTotalOrders()<<std::endl;
            auto longest_trade = getLongestTimeTrades();
            auto longest_tick  = getLongestTimeTick();
            std::cout<<"Overall Longest Time between Trades: "<<longest_trade.first<<"|"<<longest_trade.second<<" seconds"<<std::endl;
            std::cout<<"Overall Longest Time between Tick: "<<longest_tick.first<<"|"<<longest_tick.second<<" seconds"<<std::endl;
        }
        void save(const std::string& destination_file) const
        {
            std::ofstream file(destination_file);
            if (!file)
            {
                std::cerr << "Failed to open file for writing: " << destination_file << std::endl;
                return;
            }

            file << std::left << std::setw(35) << "Symbol"
                << std::setw(20) << "Mean Trade Time"
                << std::setw(20) << "Median Trade Time"
                << std::setw(20) << "Longest Trade Time"
                << std::setw(20) << "Mean Tick Time"
                << std::setw(20) << "Median Tick Time"
                << std::setw(20) << "Longest Tick Time"
                << std::setw(20) << "Mean Spread"
                << std::setw(20) << "Median Spread"
                << std::endl;

            for (const auto& pair : table)
            {
                const std::string& symbol = pair.first;
                const OrderBook& book = pair.second;

                file << std::left << std::setw(35) << symbol
                    << std::setw(20) << std::fixed << std::setprecision(6) << book.get_mean_time_trades()
                    << std::setw(20) << book.get_median_time_trades()
                    << std::setw(20) << book.get_longest_time_trades()
                    << std::setw(20) << std::fixed << std::setprecision(4) << book.get_mean_time_tick()
                    << std::setw(20) << book.get_median_time_tick()
                    << std::setw(20) << book.get_longest_time_tick()
                    << std::setw(20) << book.get_mean_spread()
                    << std::setw(20) << book.get_median_spread()
                    << std::endl;
            }

            file.close();
        }

        std::pair<std::string, double> getLongestTimeTrades() const
        {
            /* Function determines the longest time between trades among all stocks */
            std::pair<std::string, double> longestTimeTrades;

            for (const auto& pair : table)
            {
                double longestTime = pair.second.get_longest_time_trades();
                if (longestTime > longestTimeTrades.second)
                {
                    longestTimeTrades = { pair.first, longestTime };
                }
            }

            return longestTimeTrades;
        }

        std::pair<std::string, double> getLongestTimeTick() const
        {
            /* Function determines the longest time between tick among all stocks */
            std::pair<std::string, double> longestTimeTick;

            for (const auto& pair : table)
            {
                double longestTime = pair.second.get_longest_time_tick();
                if (longestTime > longestTimeTick.second)
                {
                    longestTimeTick = { pair.first, longestTime };
                }
            }

            return longestTimeTick;
        }
        ~OrderTable() {}
};