// Copyright (c) 2019-2024, The Monero Project
// 
// All rights reserved.
// 
// Redistribution and use in source and binary forms, with or without modification, are
// permitted provided that the following conditions are met:
// 
// 1. Redistributions of source code must retain the above copyright notice, this list of
//    conditions and the following disclaimer.
// 
// 2. Redistributions in binary form must reproduce the above copyright notice, this list
//    of conditions and the following disclaimer in the documentation and/or other
//    materials provided with the distribution.
// 
// 3. Neither the name of the copyright holder nor the names of its contributors may be
//    used to endorse or promote products derived from this software without specific
//    prior written permission.
// 
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY
// EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
// MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL
// THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
// PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
// STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF
// THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

#include <boost/program_options.hpp>
#include <boost/format.hpp>
#include <boost/algorithm/string.hpp>
#include <openssl/ssl.h>
#include <openssl/pem.h>
#include "include_base_utils.h"
#include "file_io_utils.h"
#include "net/net_ssl.h"
#include "crypto/crypto.h"
#include "common/util.h"
#include "common/i18n.h"
#include "common/command_line.h"
#include "common/scoped_message_writer.h"
#include "common/password.h"
#include "version.h"

namespace po = boost::program_options;

#undef MONERO_DEFAULT_LOG_CATEGORY
#define MONERO_DEFAULT_LOG_CATEGORY "gen_ssl_cert"

namespace gencert
{
  const char* tr(const char* str)
  {
    return i18n_translate(str, "tools::gen_ssl_cert");
  }

}

namespace
{
  const command_line::arg_descriptor<std::string> arg_certificate_filename = {"certificate-filename", gencert::tr("Filename to save the certificate"), ""};
  const command_line::arg_descriptor<std::string> arg_private_key_filename = {"private-key-filename", gencert::tr("Filename to save the private key"), ""};
  const command_line::arg_descriptor<std::string> arg_passphrase = {"passphrase", gencert::tr("Passphrase with which to encrypt the private key"), ""};
  const command_line::arg_descriptor<std::string> arg_passphrase_file = {"passphrase-file", gencert::tr("File containing the passphrase with which to encrypt the private key"), ""};
  const command_line::arg_descriptor<bool> arg_prompt_for_passphrase = {"prompt-for-passphrase", gencert::tr("Prompt for a passphrase with which to encrypt the private key"), false};
}

int main(int argc, char* argv[])
{
  TRY_ENTRY();

  tools::on_startup();

  po::options_description desc_cmd_only("Command line options");
  po::options_description desc_cmd_sett("Command line options and settings options");

  command_line::add_arg(desc_cmd_sett, arg_certificate_filename);
  command_line::add_arg(desc_cmd_sett, arg_private_key_filename);
  command_line::add_arg(desc_cmd_sett, arg_passphrase);
  command_line::add_arg(desc_cmd_sett, arg_passphrase_file);
  command_line::add_arg(desc_cmd_sett, arg_prompt_for_passphrase);

  command_line::add_arg(desc_cmd_only, command_line::arg_help);
  command_line::add_arg(desc_cmd_only, command_line::arg_version);

  po::options_description desc_options("Allowed options");
  desc_options.add(desc_cmd_only).add(desc_cmd_sett);

  po::variables_map vm;
  bool r = command_line::handle_error_helper(desc_options, [&]()
  {
    po::store(po::parse_command_line(argc, argv, desc_options), vm);
    po::notify(vm);
    return true;
  });
  if (!r)
    return 1;

  if (command_line::get_arg(vm, command_line::arg_help))
  {
    std::cout << "Monero '" << MONERO_RELEASE_NAME << "' (v" << MONERO_VERSION_FULL << ")" << ENDL << ENDL;
    std::cout << desc_options << std::endl;
    return 0;
  }
  if (command_line::get_arg(vm, command_line::arg_version))
  {
    std::cout << "Monero '" << MONERO_RELEASE_NAME << "' (v" << MONERO_VERSION_FULL << ")" << ENDL;
    return 0;
  }

  const std::string certificate_filename = command_line::get_arg(vm, arg_certificate_filename);
  if (certificate_filename.empty())
  {
    tools::fail_msg_writer() << gencert::tr("Argument is needed: ") << "--" << arg_certificate_filename.name;
    return 1;
  }
  const std::string private_key_filename = command_line::get_arg(vm, arg_private_key_filename);
  if (private_key_filename.empty())
  {
    tools::fail_msg_writer() << gencert::tr("Argument is needed: ") << "--" << arg_private_key_filename.name;
    return 1;
  }

  epee::wipeable_string private_key_passphrase;
  if (command_line::get_arg(vm, arg_prompt_for_passphrase))
  {
    auto pwd_container = tools::password_container::prompt(true, "Enter passphrase for the new SSL private key");
    if (!pwd_container)
    {
      tools::fail_msg_writer() << gencert::tr("Failed to read passphrase");
      return 1;
    }
    private_key_passphrase = pwd_container->password();
  }
  else if (!command_line::is_arg_defaulted(vm, arg_passphrase_file))
  {
    std::string passphrase_file = command_line::get_arg(vm, arg_passphrase_file);
    if (!passphrase_file.empty())
    {
      std::string passphrase;
      if (!epee::file_io_utils::load_file_to_string(passphrase_file, passphrase))
      { 
        MERROR("Failed to load passphrase");
        return 1;
      }

      // Remove line breaks the user might have inserted
      boost::trim_right_if(passphrase, boost::is_any_of("\r\n"));
      private_key_passphrase = passphrase;
      memwipe(&passphrase[0], passphrase.size());
    }
  }
  else
  {
    private_key_passphrase = command_line::get_arg(vm, arg_passphrase);
  }
  if (private_key_passphrase.empty())
    tools::msg_writer(epee::console_color_yellow) << (boost::format(tr("Empty passphrase, the private key will be saved to disk unencrypted, use --%s to set a passphrase or --%s to prompt for one")) % arg_passphrase.name % arg_prompt_for_passphrase.name).str();

  EVP_PKEY *pkey;
  X509 *cert;
  r = epee::net_utils::create_rsa_ssl_certificate(pkey, cert);
  if (!r)
  {
    tools::fail_msg_writer() << gencert::tr("Failed to create certificate");
    return 1;
  }

  // write cert
  BIO *bio_cert = BIO_new(BIO_s_mem());
  r = PEM_write_bio_X509(bio_cert, cert);
  if (!r)
  {
    BIO_free(bio_cert);
    tools::fail_msg_writer() << gencert::tr("Failed to write certificate: ") << ERR_reason_error_string(ERR_get_error());
    return 1;
  }
  BUF_MEM *buf = NULL;
  BIO_get_mem_ptr(bio_cert, &buf);
  if (!buf || !buf->data || !buf->length)
  {
    BIO_free(bio_cert);
    tools::fail_msg_writer() << gencert::tr("Failed to write certificate: ") << ERR_reason_error_string(ERR_get_error());
    return 1;
  }
  const std::string certificate(std::string(buf->data, buf->length));
  BIO_free(bio_cert);

  // write private key
  BIO *bio_pkey = BIO_new(BIO_s_mem());
  r = PEM_write_bio_PKCS8PrivateKey(bio_pkey, pkey, private_key_passphrase.empty() ? NULL : EVP_aes_128_cfb(), private_key_passphrase.data(), private_key_passphrase.size(), NULL, NULL);
  if (!r)
  {
    BIO_free(bio_pkey);
    tools::fail_msg_writer() << gencert::tr("Failed to write private key: ") << ERR_reason_error_string(ERR_get_error());
    return 1;
  }
  buf = NULL;
  BIO_get_mem_ptr(bio_pkey, &buf);
  if (!buf || !buf->data || !buf->length)
  {
    BIO_free(bio_pkey);
    tools::fail_msg_writer() << gencert::tr("Failed to write private key: ") << ERR_reason_error_string(ERR_get_error());
    return 1;
  }
  const std::string private_key(std::string(buf->data, buf->length));
  BIO_free(bio_pkey);

  // write files
  tools::set_strict_default_file_permissions(true);
  r = epee::file_io_utils::save_string_to_file(certificate_filename, certificate);
  if (!r)
  {
    tools::fail_msg_writer() << gencert::tr("Failed to save certificate file");
    return 1;
  }
  r = epee::file_io_utils::save_string_to_file(private_key_filename, private_key);
  if (!r)
  {
    tools::fail_msg_writer() << gencert::tr("Failed to save private key file");
    return 1;
  }

  tools::success_msg_writer() << tr("New certificate created:");
  tools::success_msg_writer() << tr("Certificate: ") << certificate_filename;
  tools::success_msg_writer() << tr("SHA-256 Fingerprint: ") << epee::net_utils::get_hr_ssl_fingerprint(cert);
  tools::success_msg_writer() << tr("Private key: ") << private_key_filename << " (" << (private_key_passphrase.empty() ? "unencrypted" : "encrypted") << ")";

  return 0;
  CATCH_ENTRY_L0("main", 1);
}
