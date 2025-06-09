/*
    A logger is a utility that:
        1. Record messages at different severity levels (debug, info warning, error, critical, etc)
        2. Can output to various destinations like console, file, network, etc
        3. often includes timestamps and toher contextual information
        4. Allows configuration of verbosity levels
*/

#include <iostream>
#include <fstream>
#include <mutex>
#include <string>
#include <chrono>
#include <iomanip>

// Multiple thread to same file
#include <thread>
#include <vector>

enum class LogLevel{
    DEBUG,
    INFO,
    WARNING,
    ERROR,
    CRITICAL
};

class Logger{
private:
    std::ofstream logfile;
    LogLevel minimumLevel;
    std::mutex logMutex; // for thread safety

    // Get current time as string
    std::string getCurrentTime() {
        auto now = std::chrono::system_clock::now();
        auto in_time_t = std::chrono::system_clock::to_time_t(now);

        std::stringstream ss;
        ss << std::put_time(std::localtime(&in_time_t), "%Y-%m-%d %X");
        return ss.str();
    }

    // Convert log levels to string
    std::string levelToString(LogLevel level){
        switch(level){
            case LogLevel::DEBUG : return "DEBUG";
            case LogLevel::INFO : return "INFO";
            case LogLevel::WARNING : return "WARNING";
            case LogLevel::ERROR : return "ERROR";
            case LogLevel::CRITICAL : return "CRITICAL";
            default : return "UNKNOWN";
        }
    }

public:
    // Constructor Initialize the logfile
    Logger(const std::string & filename = "app.log", LogLevel minLevel = LogLevel::INFO) : minimumLevel(minLevel) {
        logfile.open(filename, std::ios::out | std::ios::app);
        if(!logfile.is_open()) std::cerr << "Failed to open the log file : "<<filename<<std::endl;
    }

    // Destructor - close the logfile
    ~Logger(){
        if(logfile.is_open()) logfile.close();
    }

    // Main logging function
    void log(LogLevel level, const std::string &message){
        if(level < minimumLevel) return;
        // for lock scope
        {
            std::lock_guard<std::mutex> lock(logMutex); // Thread safety
            
            std::string logEntry = "[" + getCurrentTime() + "]" + "[" + levelToString(level) + "]" + message;

            // output to consure;
            std::cout<< logEntry<<std::endl; // shared resource 1

            //output to file if open
            if(logfile.is_open()){
                logfile << logEntry << std::endl; // shared resource 2
            }
        }
    }

    // Using methods for different log levels;
    void debug(const std::string &message) {log(LogLevel::DEBUG, message);}
    void info(const std::string &message) {log(LogLevel::INFO, message);}
    void warning(const std::string &message) {log(LogLevel::WARNING, message);}
    void error(const std::string &message) {log(LogLevel::ERROR, message);}
    void critical(const std::string &message) {log(LogLevel::CRITICAL, message);}
};

void SingleThreadLogging(Logger &logger, const std::string &logFilename){
    logger.info("----- IN SINGLE THREAD LOGIC ------");
    logger.debug("This is debug message");
    logger.info("Application started and logfile name is : " + logFilename);
    logger.warning("Low memory condition detected");
    logger.error("Failed some where");
    logger.critical("Crash detected");
    logger.info("----- END SINGLE THREAD LOGIC ------");
}

// Thread function which will log multiple messages
void threadFunction(Logger &logger, int threadid) {
    for(int i=0; i<5; i++){
        std::string msg = "Thread : " + std::to_string(threadid) + " - Message " + std::to_string(i);
        // i am writing two calls to my logger, which means two are independent and any thread can come in between
        logger.info(msg);
        logger.debug("In threading debug - The thread is :" + std::to_string(threadid));
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
}

int main(int argc, char *argv[]) {
    // default log file name
    std::string logFilename = "myLogs.txt";

    if(argc > 1) logFilename = argv[1];
    
    Logger logger(logFilename, LogLevel::DEBUG); // LogFile name and minimum log level

    // Single thread logger
    SingleThreadLogging(logger, logFilename);

    const int numThreads = 5;
    std::vector<std::thread> threads;

    // create and launch threads
    logger.info("----- IN MULTI THREAD LOGIC -----");
    for(int i=0;i < numThreads; i++){
        threads.emplace_back(threadFunction, std::ref(logger), i+1);
    }

    //wait for all threads to finish
    for(auto& thread : threads) {
        thread.join();
    }

    logger.info("------ ALL THREADS ARE COMPLETED ------");

    return 0;
}