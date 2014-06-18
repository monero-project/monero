
namespace Monero {
    
    namespace Errors {

        class InvalidPassword: public std::exception
        {
        public:
            virtual const char* what() const throw()
            {
                return "Wrong wallet password";
            }
        } iInvalidPassword;


        class InvalidFile: public std::exception
        {
        public: 
            virtual const char* what() const throw()
            {
                return "Wallet file not found or invalid";
            }
        } iInvalidFile;


        class NotWritableFile: public std::exception
        {
        public: 
            virtual const char* what() const throw()
            {
                return "Unable to write file";
            }

        } iNotWritableFile;


        class InvalidAddress: public std::exception
        {
        public: 
            virtual const char* what() const throw()
            {
                return "Address should be composed of 95 alphanumerics characters";
            }

        } iInvalidAddress;


        class InvalidPaymentID: public std::exception
        {
        public: 
            virtual const char* what() const throw()
            {
                return "Payment ID should be a Hex of 32 bits (64 chars hex string)";
            }

        } iInvalidPaymentID;


        class InvalidNonce: public std::exception
        {
        public: 
            virtual const char* what() const throw()
            {
                return "Invalid Nonce";
            }

        } iInvalidNonce;


        class NoDaemonConnection: public std::exception
        {
        public: 
            virtual const char* what() const throw()
            {
                return "No connection to daemon. Use 'connect()' first. And check that 'bitmonerod' is running";
            }

        } iNoDaemonConnection;


        class DaemonBusy: public std::exception
        {
        public: 
            virtual const char* what() const throw()
            {
                return "Daemon is busy. Please wait for blockchain operations and try again";
            }

        } iDaemonBusy;
    }
}
