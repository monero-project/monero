#pragma once

namespace Monero {
    
    namespace Errors {

        class InvalidPassword: public std::exception
        {
        public:
            virtual const char* what() const throw()
            {
                return "Wrong wallet password";
            }
        };


        class InvalidFile: public std::exception
        {
        public: 
            virtual const char* what() const throw()
            {
                return "Wallet file not found or invalid";
            }
        };


        class NotWritableFile: public std::exception
        {
        public: 
            virtual const char* what() const throw()
            {
                return "Unable to write file";
            }

        };

        class InvalidAddress: public std::exception
        {
        public: 
            virtual const char* what() const throw()
            {
                return "Address should be composed of 95 alphanumerics characters";
            }

        };


        class InvalidPaymentID: public std::exception
        {
        public: 
            virtual const char* what() const throw()
            {
                return "Payment ID should be a Hex of 32 bits (64 chars hex string)";
            }

        };


        class InvalidNonce: public std::exception
        {
        public: 
            virtual const char* what() const throw()
            {
                return "Invalid Nonce";
            }

        };


        class NoDaemonConnection: public std::exception
        {
        public: 
            virtual const char* what() const throw()
            {
                return "No connection to daemon. Use 'connect()' first. And check that 'bitmonerod' is running";
            }

        };


        class DaemonBusy: public std::exception
        {
        public: 
            virtual const char* what() const throw()
            {
                return "Daemon is busy. Please wait for blockchain operations and try again";
            }

        };

        class WalletInternalError
        {
        public: 
            virtual const char* what() const throw()
            {
                return "Unknown Internal error. Restarting the application could resolve the problem.";
            }

        };


        /* Transfer errors */
        class TransactionTooBig: public std::exception
        {
        public: 
            virtual const char* what() const throw()
            {
                return "Your transaction is too big";
            }
        };

        class TransactionZeroDestination: public std::exception
        {
        public: 
            virtual const char* what() const throw()
            {
                return "No destination or invalid destination(s)";
            }
        };


        class TransactionSumOverflow: public std::exception
        {
        public: 
            virtual const char* what() const throw()
            {
                return "Transaction sum overflow (too much money ?)";
            }
        };


        class TransactionNotEnoughMoney: public std::exception
        {
        public: 
            virtual const char* what() const throw()
            {
                return "Not enough money for this transaction";
            }
        };


        class TransactionNotEnoughOuts: public std::exception
        {
        public: 
            virtual const char* what() const throw()
            {
                return "Not enough output transactions for mixin";
            }
        };


        class TransactionUnexpectedType: public std::exception
        {
        public: 
            virtual const char* what() const throw()
            {
                return "Internal error : Wrong transaction type";
            }
        };


        class TransactionRejected: public std::exception
        {
        public: 
            virtual const char* what() const throw()
            {
                return "Your transaction was rejected by Daemon. Aborted.";
            }
        };






    }
}
