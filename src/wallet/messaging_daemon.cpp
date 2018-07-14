
#include "messaging_daemon.h"
#include "common/i18n.h"
#include "rpc/rpc_args.h"
#include "common/command_line.h"
#include <boost/optional/optional.hpp>
#include <boost/chrono.hpp>
#include <boost/format.hpp>
#include "string_coding.h"

#undef MONERO_DEFAULT_LOG_CATEGORY
#define MONERO_DEFAULT_LOG_CATEGORY "wallet.mms"

namespace
{
  const command_line::arg_descriptor<std::string> arg_rpc_bind_port = {"rpc-bind-port", "Sets bind port for messaging daemon", "18083"};
  const command_line::arg_descriptor<std::string> arg_bitmessage_url = {"bitmessage-url", "Sets the URL of the PyBitmessage client", "http://localhost:8442/"};
  const command_line::arg_descriptor<std::string> arg_bitmessage_user = {"bitmessage-user", "Sets the user of the PyBitmessage client", "username"};
  const command_line::arg_descriptor<std::string> arg_bitmessage_password = {"bitmessage-password", "Sets the password of the PyBitmessage client", "password"};
}

namespace tools
{

const char* messaging_daemon::tr(const char* str)
{
  return i18n_translate(str, "tools::messaging_daemon");
}
//------------------------------------------------------------------------------------------------------------------------------
messaging_daemon::messaging_daemon(): m_vm(NULL)
{
}
//------------------------------------------------------------------------------------------------------------------------------
messaging_daemon::~messaging_daemon()
{
}

bool messaging_daemon::run()
{
  m_stop = false;
  m_net_server.add_idle_handler([this](){
    if (m_stop.load(std::memory_order_relaxed))
    {
      send_stop_signal();
      return false;
    }
    return true;
  }, 500);

  return epee::http_server_impl_base<messaging_daemon, connection_context>::run(1, true);
}
//------------------------------------------------------------------------------------------------------------------------------
void messaging_daemon::stop()
{
}
//------------------------------------------------------------------------------------------------------------------------------
bool messaging_daemon::init(const boost::program_options::variables_map *vm)
{
  auto rpc_config = cryptonote::rpc_args::process(*vm);
  if (!rpc_config)
  {
    return false;
  }

  m_vm = vm;

  boost::optional<epee::net_utils::http::login> http_login{};

  std::string bind_port = command_line::get_arg(*m_vm, arg_rpc_bind_port);  
  auto rng = [](size_t len, uint8_t *ptr) { return crypto::rand(len, ptr); };
  m_net_server.set_threads_prefix("RPC");
  bool daemon_connected = epee::http_server_impl_base<messaging_daemon, connection_context>::init(
    rng, std::move(bind_port), std::move(rpc_config->bind_ip), std::move(rpc_config->access_control_origins), std::move(http_login)
  );

  m_bitmessage_url = command_line::get_arg(*m_vm, arg_bitmessage_url);
  m_bitmessage_user = command_line::get_arg(*m_vm, arg_bitmessage_user);
  m_bitmessage_password = command_line::get_arg(*m_vm, arg_bitmessage_password);
  boost::optional<epee::net_utils::http::login> bitmessage_login{};
  bitmessage_login.emplace(m_bitmessage_user, m_bitmessage_password);

  m_http_client.set_server(m_bitmessage_url, bitmessage_login, false);
  
  return true;
}

bool messaging_daemon::on_add(const messaging_daemon_rpc::COMMAND_RPC_ADD::request& req, messaging_daemon_rpc::COMMAND_RPC_ADD::response& res, epee::json_rpc::error& er)
{
  /*
  try
  {
    res.sum = req.first + req.second;
  }
  */
  try
  {
    std::string request =
      "<?xml version=\"1.0\"?> \
      <methodCall> \
	<methodName>add</methodName> \
	<params> \
	  <param> \
	      <value><i4>3</i4></value> \
	  </param> \
	  <param> \
	      <value><i4>4</i4></value> \
	  </param> \
	</params> \
      </methodCall> \
      ";
    /* 
    request =
      "<?xml version=\"1.0\"?> \
      <methodCall> \
	<methodName>getAllInboxMessages</methodName> \
	<params> \
	</params> \
      </methodCall> \
      ";
    // 1494252ecf5531d0abac6fd66c5d2d6ddab1ad1a3a92954d215250a95e599217
    */
    std::string answer;
    post_request(request, answer);
    /*
    std::string json = get_str_between_tags(answer, "<string>", "</string>");
    bitmessage_rpc::inbox_messages_response messages;
    epee::serialization::load_t_from_json(messages, json);
    size_t size = messages.inboxMessages.size();
    if (size > 0)
    {
      bitmessage_rpc::message_info info = messages.inboxMessages.front();
    }
    */
  }
  catch (const std::exception& e)
  {
    handle_rpc_exception(std::current_exception(), er, MESSAGING_DAEMON_ERROR_CODE_UNKNOWN_ERROR);
    return false;
  }
  return true;
}

bool messaging_daemon::on_send_message(const messaging_daemon_rpc::COMMAND_RPC_SEND_MESSAGE::request& req, messaging_daemon_rpc::COMMAND_RPC_SEND_MESSAGE::response& res, epee::json_rpc::error& er)
{
  // <toAddress> <fromAddress> <subject> <message> [encodingType [TTL]]
  try {
    std::string request;
    start_xml_rpc_cmd(request, "sendMessage");
    add_xml_rpc_string_param(request, req.msg.destination_transport_address);
    add_xml_rpc_string_param(request, req.msg.source_transport_address);
    add_xml_rpc_base64_param(request, "MMS");
    std::string json = epee::serialization::store_t_to_json(req.msg);
    std::string message_body = epee::string_encoding::base64_encode(json);  // See comment in "on_receive_message" about reason for (double-)Base64 encoding
    add_xml_rpc_base64_param(request, message_body);
    add_xml_rpc_integer_param(request, 2);
    end_xml_rpc_cmd(request);
    std::string answer;
    post_request(request, answer);
  }
  catch (const std::exception& e)
  {
    handle_rpc_exception(std::current_exception(), er, MESSAGING_DAEMON_ERROR_CODE_UNKNOWN_ERROR);
    return false;
  }
  return true;
}

bool messaging_daemon::on_receive_messages(const messaging_daemon_rpc::COMMAND_RPC_RECEIVE_MESSAGES::request& req, messaging_daemon_rpc::COMMAND_RPC_RECEIVE_MESSAGES::response& res, epee::json_rpc::error& er)
{
  // The message body of the Bitmessage message is basically the messaging daemon message, as JSON (and nothing more)
  // Weeding out other, non-MMS messages is done in a simple way: If it deserializes without error, it's an MMS message
  // That JSON is Base64-encoded by the MMS because the Monero epee JSON serializer does not escape anything and happily
  // includes even 0 (NUL) in strings, which might confuse Bitmessage and would display confusingly in the client
  // There is yet another Base64-encoding of course as part of the Bitmessage API for the message body parameter
  // The Bitmessage API call "getAllInboxMessages" gives back a JSON array with all the messages (despite using
  // XML-RPC for the calls, and not JSON-RPC ...)
  try
  {
    std::string request;
    start_xml_rpc_cmd(request, "getAllInboxMessages");
    end_xml_rpc_cmd(request);
    std::string answer;
    post_request(request, answer);
    
    std::string json = get_str_between_tags(answer, "<string>", "</string>");
    bitmessage_rpc::inbox_messages_response bitmessage_res;
    epee::serialization::load_t_from_json(bitmessage_res, json);
    size_t size = bitmessage_res.inboxMessages.size();
    res.msg.clear();
    
    for (size_t i = 0; i < size; ++i)
    {
      bitmessage_rpc::message_info message_info = bitmessage_res.inboxMessages[i];
      messaging_daemon_rpc::message message;
      bool is_mms_message = false;
      try
      {
	// First Base64-decoding: The message body is Base64 in the Bitmessage API
	std::string message_body = epee::string_encoding::base64_decode(message_info.message);
	// Second Base64-decoding: The MMS uses Base64 to hide non-textual data in its JSON from Bitmessage
	json = epee::string_encoding::base64_decode(message_body);
        epee::serialization::load_t_from_json(message, json);
	is_mms_message = true;
      }
      catch(const std::exception& e)
      {
      }
      if (is_mms_message)
      {
	if (message.destination_monero_address == req.destination_monero_address)
	{
	  message.transport_id = message_info.msgid;
	  res.msg.push_back(message);
	}
      }
    }

  }
  catch (const std::exception& e)
  {
    handle_rpc_exception(std::current_exception(), er, MESSAGING_DAEMON_ERROR_CODE_UNKNOWN_ERROR);
    return false;
  }
  return true;
}

bool messaging_daemon::on_delete_message(const messaging_daemon_rpc::COMMAND_RPC_DELETE_MESSAGE::request& req, messaging_daemon_rpc::COMMAND_RPC_DELETE_MESSAGE::response& res, epee::json_rpc::error& er)
{
  try {
    std::string request;
    start_xml_rpc_cmd(request, "trashMessage");
    add_xml_rpc_string_param(request, req.transport_id);
    end_xml_rpc_cmd(request);
    std::string answer;
    post_request(request, answer);
  }
  catch (const std::exception& e)
  {
    handle_rpc_exception(std::current_exception(), er, MESSAGING_DAEMON_ERROR_CODE_UNKNOWN_ERROR);
    return false;
  }
  return true;
}

void messaging_daemon::handle_rpc_exception(const std::exception_ptr& e, epee::json_rpc::error& er, int default_error_code)
{
  try
  {
    std::rethrow_exception(e);
  }
  catch (const std::exception& e)
  {
    er.code = default_error_code;
    er.message = e.what();
  }
  catch (...)
  {
    er.code = MESSAGING_DAEMON_ERROR_CODE_UNKNOWN_ERROR;
    er.message = "MESSAGING_DAEMON_ERROR_CODE_UNKNOWN_ERROR";
  }
}

std::string messaging_daemon::get_str_between_tags(const std::string &s, const std::string &start_delim, const std::string &stop_delim)
{
    size_t first_delim_pos = s.find(start_delim);
    size_t end_pos_of_first_delim = first_delim_pos + start_delim.length();
    size_t last_delim_pos = s.find(stop_delim);
    return s.substr(end_pos_of_first_delim, last_delim_pos - end_pos_of_first_delim);
}

bool messaging_daemon::post_request(const std::string &request, std::string &answer)
{
  // Somehow things do not work out if one tries to connect "m_http_client" to Bitmessage
  // and keep it connected over the course of several calls. But with a new connection per
  // call and disconnecting after the call there is no problem (despite perhaps a small
  // slowdown)
  epee::net_utils::http::fields_list additional_params;
  
  // Basic access authentication according to RFC 7617 (which the epee HTTP classes do not seem to support?)
  std::string user_password(m_bitmessage_user + ":" + m_bitmessage_password);
  std::string auth_string = epee::string_encoding::base64_encode(user_password);
  auth_string.insert(0, "Basic ");
  additional_params.push_back(std::make_pair("Authorization", auth_string));
  
  additional_params.push_back(std::make_pair("Content-Type", "application/xml; charset=utf-8"));
  const epee::net_utils::http::http_response_info* response = NULL;
  std::chrono::milliseconds timeout = std::chrono::seconds(15);
  bool r = m_http_client.invoke("/", "POST", request, timeout, std::addressof(response), std::move(additional_params));
  if (r)
  {
    answer = response->m_body;
  }
  else
  {
    LOG_ERROR("POST request to Bitmessage failed: " << request);
  }
  m_http_client.disconnect();  // see comment above
  
  return r;
}


void messaging_daemon::start_xml_rpc_cmd(std::string &xml, const std::string &method_name)
{
  xml = (boost::format("<?xml version=\"1.0\"?><methodCall><methodName>%s</methodName><params>") % method_name).str();
}

void messaging_daemon::add_xml_rpc_string_param(std::string &xml, const std::string &param)
{
  xml += (boost::format("<param><value><string>%s</string></value></param>") % param).str();
}

void messaging_daemon::add_xml_rpc_base64_param(std::string &xml, const std::string &param)
{
  // Bitmessage expects some arguments Base64-encoded, but it wants them as parameters of type "string", not "base64" that is also part of XML-RPC
  std::string encoded_param = epee::string_encoding::base64_encode(param);
  xml += (boost::format("<param><value><string>%s</string></value></param>") % encoded_param).str();
}

void messaging_daemon::add_xml_rpc_integer_param(std::string &xml, const int32_t &param)
{
  xml += (boost::format("<param><value><int>%i</int></value></param>") % param).str();
}

void messaging_daemon::end_xml_rpc_cmd(std::string &xml)
{
  xml += "</params></methodCall>";
}


}

int main(int argc, char** argv) {
  namespace po = boost::program_options;
  
  tools::on_startup();

  po::options_description desc_params("messaging_daemon");
  cryptonote::rpc_args::init_options(desc_params);
  command_line::add_arg(desc_params, arg_rpc_bind_port);
  command_line::add_arg(desc_params, arg_bitmessage_url);
  command_line::add_arg(desc_params, arg_bitmessage_user);
  command_line::add_arg(desc_params, arg_bitmessage_password);
  po::variables_map vm;
  
  auto parser = po::command_line_parser(argc, argv).options(desc_params);
  po::store(parser.run(), vm);
  po::notify(vm);

  /*
  bool r = command_line::handle_error_helper(desc_params, [&]()
  {
    po::store(po::parse_command_line(argc, argv, desc_params), vm);
    po::notify(vm);
    return true;
  });
  if (!r)
  {
    return 1;
  }
  */
  
  tools::messaging_daemon ms;
  bool r = ms.init(&(vm));
  CHECK_AND_ASSERT_MES(r, 1, tr("Failed to initialize messaging daemon"));
  tools::signal_handler::install([&ms](int) {
    ms.send_stop_signal();
  });
  LOG_PRINT_L0(tr("Starting messaging_daemon"));
  try
  {
    ms.run();
  }
  catch (const std::exception &e)
  {
    LOG_ERROR(tr("Failed to run messaging daemon") << e.what());
    return 1;
  }
  LOG_PRINT_L0(tr("Stopped messaging daemon"));
  
  return 0;
}
