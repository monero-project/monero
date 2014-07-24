
#include <>

/* implementation for the non-template base, for the connection<> template class */


// ################################################################################################
// ################################################################################################
// the "header part". Not separeted out for .hpp because point of this modification is 
// to rebuild just 1 translation unit while working on this code.
// (But maybe common parts will be separated out later though - if needed)
// ################################################################################################
// ################################################################################################

class cryptonote_protocol_handler_base_pimpl { 
	public:

};

// ################################################################################################
// ################################################################################################
// ################################################################################################
// ################################################################################################

cryptonote_protocol_handler_base::cryptonote_protocol_handler_base() {
}

cryptonote_protocol_handler_base::~cryptonote_protocol_handler_base() {
}

void cryptonote_protocol_handler::handler_request_blocks_now(size_t count_limit) {
}

void cryptonote_protocol_handler::handler_request_blocks_history(std::list<crypto::hash>& ids) {
}


