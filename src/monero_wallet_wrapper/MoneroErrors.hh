
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
                return "Payment ID should be a Hex of 32 bits";
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
    }
}
