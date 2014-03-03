// Copyright (c) 2006-2013, Andrey N. Sabelnikov, www.sabelnikov.net
// All rights reserved.
// 
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
// * Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
// * Redistributions in binary form must reproduce the above copyright
// notice, this list of conditions and the following disclaimer in the
// documentation and/or other materials provided with the distribution.
// * Neither the name of the Andrey N. Sabelnikov nor the
// names of its contributors may be used to endorse or promote products
// derived from this software without specific prior written permission.
// 
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
// ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
// WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
// DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER  BE LIABLE FOR ANY
// DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
// (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
// LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
// ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
// SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
// 



#ifndef _MUNIN_CONNECTION_HANDLER_H_
#define _MUNIN_CONNECTION_HANDLER_H_

#include <string>
#include "net_utils_base.h"
#include "to_nonconst_iterator.h"
#include "http_base.h"
#include "reg_exp_definer.h"

#define MUNIN_ARGS_DEFAULT(vertial_lable_str) "graph_args --base 1000 -l 0 --vertical-label " vertial_lable_str " \n"
#define MUNIN_ARGS_FORCE_AUPPER_LIMIT(vertial_lable_str, limit) "graph_args --base 1000 -l 0 --vertical-label " vertial_lable_str " --rigid --upper-limit " limit " \n"
#define MUNIN_TITLE(title_str) "graph_title " title_str "\n"
#define MUNIN_CATEGORY(category_str) "graph_category " category_str "\n"
#define MUNIN_INFO(info_str) "graph_info " info_str "\n"
#define MUNIN_ENTRY(var_name) #var_name".label " #var_name "\n" #var_name".info "#var_name".\n"
#define MUNIN_ENTRY_AREA(var_name) #var_name".label " #var_name "\n" #var_name".info "#var_name".\n" #var_name".draw AREASTACK\n"
#define MUNIN_ENTRY_ALIAS(var_name, alias) #var_name".label " #alias"\n" #var_name".info "#alias".\n"
#define BEGIN_MUNIN_SERVICE(servivece_name_str) if(servivece_name_str == pservice->m_service_name) {
#define END_MUNIN_SERVICE() }
#define MUNIN_SERVICE_PARAM(munin_var_name_str, variable) paramters_text += std::string() + munin_var_name_str ".value " + boost::lexical_cast<std::string>(variable) + "\n"




namespace epee
{
namespace net_utils
{
	namespace munin
	{
	

		/************************************************************************/
		/*                                                                      */
		/************************************************************************/
		struct munin_service;

		struct munin_service_data_provider
		{
			virtual bool update_service_data(munin_service* pservice, std::string& paramters_text)=0;
		};

		struct munin_service
		{
			std::string m_service_name;
			std::string m_service_config_string;
			munin_service_data_provider* m_pdata_provider;
		};

		struct node_server_config
		{
			std::list<munin_service> m_services;
			//TODO:
		};

		struct fake_send_handler: public i_service_endpoint
		{
			virtual bool do_send(const void* ptr, size_t cb)
			{
				m_cache += std::string((const char*)ptr, cb);
				return true;
			}
		public:

			std::string m_cache;
		};

		/************************************************************************/
		/*                                                                      */
		/************************************************************************/
		class munin_node_server_connection_handler
		{
		public:
			typedef node_server_config config_type;
      typedef connection_context_base connection_context;

			munin_node_server_connection_handler(i_service_endpoint* psnd_hndlr, config_type& config, const connection_context_base& context):m_psnd_hndlr(psnd_hndlr), 
												m_machine_state(http_state_retriving_comand_line), 
												m_config(config)
			{
				init();
			}
			virtual ~munin_node_server_connection_handler()
			{

			}

      bool release_protocol()
      {
        return true;
      }
			bool after_init_connection()
			{
				std::string hello_str = "# munin node at ";
				hello_str += m_host_name + "\n";
				send_hook(hello_str);
				return true;
			}

			virtual bool thread_init()
			{
				return true;
			}

			virtual bool thread_deinit()
			{
				return true;
			}

      void handle_qued_callback()
      {

      }

			virtual bool handle_recv(const void* ptr, size_t cb)
			{
				
				const char* pbuff = (const char*)ptr;
				std::string recvd_buff(pbuff, cb);
				LOG_PRINT("munin_recv: \n" << recvd_buff, LOG_LEVEL_3);

				m_cache += recvd_buff;

				bool stop_handling = false;
				while(!stop_handling)
				{
					switch(m_machine_state)
					{
					case http_state_retriving_comand_line:
						{
							
							std::string::size_type fpos = m_cache.find('\n');
							if(std::string::npos != fpos )
							{
								bool res = handle_command(m_cache);
								if(!res)
									return false;
								m_cache.erase(0, fpos+1);
								continue;
							}
							stop_handling = true;
						}
						break;
					case http_state_error:
						stop_handling = true;
						return false;
					default:
						LOG_ERROR("Error in munin state machine! Unkonwon state=" << m_machine_state);
						stop_handling = true;
						m_machine_state = http_state_error;
						return false;
					}

				}
				
				return true;
			}

		private:


			bool init()
			{
				char hostname[64] = {0};
				int res = gethostname(hostname, 64);
				hostname[63] = 0;//be happy
				m_host_name = hostname;
				return true;
			}
			bool handle_command(const std::string& command)
			{
				// list, nodes, config, fetch, version or quit
				STATIC_REGEXP_EXPR_1(rexp_match_command_line, "^((list)|(nodes)|(config)|(fetch)|(version)|(quit))(\\s+(\\S+))?", boost::regex::icase | boost::regex::normal);
				//											    12      3       4        5       6         7      8    9         
				size_t match_len = 0;
				boost::smatch result;	
				if(boost::regex_search(command, result, rexp_match_command_line, boost::match_default) && result[0].matched)
				{
					if(result[2].matched)
					{//list command
						return handle_list_command();
					}else if(result[3].matched)
					{//nodes command
						return handle_nodes_command();
					}else if(result[4].matched)
					{//config command
						if(result[9].matched)
							return handle_config_command(result[9]);
						else
						{
							send_hook("Unknown service\n");
						}
					}else if(result[5].matched)
					{//fetch command
						if(result[9].matched)
							return handle_fetch_command(result[9]);
						else
						{
							send_hook("Unknown service\n");
						}
					}else if(result[6].matched)
					{//version command
						return handle_version_command();
					}else if(result[7].matched)
					{//quit command
						return handle_quit_command();
					}
					else
						return send_hook("Unknown command. Try list, nodes, config, fetch, version or quit\n");
				}

				return send_hook("Unknown command. Try list, nodes, config, fetch, version or quit\n");;
			}

			bool handle_list_command()
			{
				std::string buff_to_send;
				for(std::list<munin_service>::const_iterator it = m_config.m_services.begin(); it!=m_config.m_services.end();it++)
				{
					buff_to_send += it->m_service_name + " ";
				}
				buff_to_send+='\n';
				return send_hook(buff_to_send);
			}
			bool handle_nodes_command()
			{
				//supports only one node - host name 
				send_hook(m_host_name + "\n.\n");
				return true;
			}
			bool handle_config_command(const std::string& service_name)
			{
				munin_service* psrv = get_service_by_name(service_name);
				if(!psrv)
					return send_hook(std::string() + "Unknown service\n");

				
				return send_hook(psrv->m_service_config_string + ".\n");
			}

			bool handle_fetch_command(const std::string& service_name)
			{
				munin_service* psrv = get_service_by_name(service_name);
				if(!psrv)
					return send_hook(std::string() + "Unknown service\n");
			
				std::string buff;
				psrv->m_pdata_provider->update_service_data(psrv, buff);

				buff += ".\n";
				return send_hook(buff);
			}
			bool handle_version_command()
			{
				return send_hook("Munin node component by Andrey Sabelnikov\n");
			}
			bool handle_quit_command()
			{
				return false;
			}

			bool send_hook(const std::string& buff)
			{
				LOG_PRINT("munin_send: \n" << buff, LOG_LEVEL_3);

				if(m_psnd_hndlr)
					return m_psnd_hndlr->do_send(buff.data(), buff.size());
				else 
					return false;
			}


			munin_service* get_service_by_name(const std::string& srv_name)
			{
				std::list<munin_service>::iterator it = m_config.m_services.begin();
				for(; it!=m_config.m_services.end(); it++)
					if(it->m_service_name == srv_name)
						break;

				if(it==m_config.m_services.end())
					return NULL;

			   return &(*it);
			}

			enum machine_state{
				http_state_retriving_comand_line,
				http_state_error
			};


			config_type& m_config;
			machine_state m_machine_state;
			std::string m_cache;
			std::string m_host_name;
		protected:
			i_service_endpoint* m_psnd_hndlr; 
		};


		inline bool test_self()
		{
			/*WSADATA w;
			::WSAStartup(MAKEWORD(1, 1), &w);
			node_server_config sc;
			sc.m_services.push_back(munin_service());
			sc.m_services.back().m_service_name = "test_service";
			
			sc.m_services.back().m_service_config_string =     
				"graph_args --base 1000 -l 0 --vertical-label N --upper-limit 329342976\n"
				"graph_title REPORTS STATICTICS\n"
				"graph_category bind\n"
				"graph_info This graph shows how many reports came in fixed time period.\n"
				"graph_order apps free swap\n"
				"apps.label apps\n"
				"apps.draw AREA\n"
				"apps.info Memory used by user-space applications.\n"
				"swap.label swap\n"
				"swap.draw STACK\n"
				"swap.info Swap space used.\n"
				"free.label unused\n"
				"free.draw STACK\n"
				"free.info Wasted memory. Memory that is not used for anything at all.\n"
				"committed.label committed\n"
				"committed.draw LINE2\n"
				"committed.warn 625410048\n"
				"committed.info The amount of memory that would be used if all the memory that's been allocated were to be used.\n";


			sc.m_services.push_back(munin_service());
			sc.m_services.back().m_service_name = "test_service1";
			fake_send_handler fh;
			munin_node_server_connection_handler mh(&fh, sc);
			
			std::string buff = "list\n";
			mh.handle_recv(buff.data(), buff.size());
			

			buff = "nodes\n";
			mh.handle_recv(buff.data(), buff.size());
*/
			return true;
		}

	}
}
}
#endif//!_MUNIN_CONNECTION_HANDLER_H_