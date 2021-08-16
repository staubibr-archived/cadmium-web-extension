/**
 * Copyright (c) 2017, Bruno St-Aubin
 * Carleton University
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 * 1. Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef WEB_MODEL_HPP
#define WEB_MODEL_HPP

#include <vector>
#include <cadmium/modeling/dynamic_message_bag.hpp>
#include <cadmium/engine/pdevs_dynamic_link.hpp>

#include <cadmium/modeling/dynamic_model.hpp>
#include <cadmium/modeling/dynamic_coupled.hpp>
#include <cadmium/modeling/dynamic_atomic.hpp>
#include <boost/algorithm/string/join.hpp>

#include "./output/message_type.hpp"

#include "./web_logger.hpp"
#include "./web_tools.hpp"

using namespace cadmium::web::output;
using namespace std;

namespace cadmium {
    namespace web {

		template<typename MSG>
		struct web_out_port:cadmium::out_port<MSG> {
			message_type get_message_type() {
				return MSG::get_message_type();
			}
		};

		template<typename MSG>
		struct web_in_port:cadmium::in_port<MSG> {
			message_type get_message_type() {
				return MSG::get_message_type();
			}
		};

		/**
		 * @brief Empty class to allow pointer based polymorphism between classes derived from
		 * atomic_web and coupled_web models.
		 */
		class web_extension {
		private:
			string _m_class = "";

		private:
            virtual vector<string> get_port_names(vector<type_index> ports) {
            	vector<string> names = {};

				for (type_index port : ports) names.push_back(tools::clean_name(port.name()));

            	return names;
            };


		public:
			virtual string get_id() = 0;

			virtual string get_class() = 0;

			virtual string get_type() = 0;

			virtual message_type get_state_message_type() = 0;

            virtual map<string, message_type> get_output_ports_message_types() = 0;

            virtual map<string, message_type> get_input_ports_message_types() = 0;

			virtual vector<shared_ptr<cadmium::dynamic::modeling::model>> get_models() = 0;

			virtual vector<cadmium::dynamic::modeling::EIC> get_eics() = 0;

			virtual vector<cadmium::dynamic::modeling::EOC> get_eocs() = 0;

			virtual vector<cadmium::dynamic::modeling::IC> get_ics() = 0;

			virtual ~web_extension() {}
		};

		template<template<typename T> class ATOMIC, typename TIME, typename... Args>
		class atomic_web : public cadmium::dynamic::modeling::atomic<ATOMIC, TIME, Args...>, public web_extension {

		private:
			string _m_class = "";

			template <typename PORTS>
			map<string, message_type> get_ports(vector<type_index> raw_names) {
				int i = 0;
				PORTS ports;
				map<string, message_type> mts;

                cadmium::helper::for_each<PORTS>(ports, [&i, &raw_names, &mts] (auto &p) -> void {
		    		string name = tools::clean_name(raw_names[i++].name());

                	mts.insert({ name, p.get_message_type() });
                });

				return mts;
			}

		public:
            using model_type=ATOMIC<TIME>;

			using output_ports = typename cadmium::dynamic::modeling::atomic<ATOMIC, TIME, Args...>::output_ports;
			using input_ports = typename cadmium::dynamic::modeling::atomic<ATOMIC, TIME, Args...>::input_ports;

			atomic_web() :
				cadmium::dynamic::modeling::atomic<ATOMIC, TIME, Args...>() { }

			atomic_web(const string& model_id, string m_class, Args&&... args) :
				cadmium::dynamic::modeling::atomic<ATOMIC, TIME, Args...>(model_id, forward<Args>(args)...)  {
				_m_class = m_class;
			}

			string get_id() override {
				return cadmium::dynamic::modeling::atomic<ATOMIC, TIME, Args...>::get_id();
			};

			string get_class() override {
				return _m_class;
			}

			string get_type() override {
				return "atomic";
			}

			message_type get_state_message_type() override {
                // Forwards the translated value to the wrapped model_type class method.
                return model_type::get_state_message_type();
			};

			map<string, message_type> get_output_ports_message_types() override {
            	vector<type_index> raw_names = cadmium::dynamic::modeling::atomic<ATOMIC, TIME, Args...>::get_output_ports();

            	return get_ports<output_ports>(raw_names);
            };

			map<string, message_type> get_input_ports_message_types() override {
            	vector<type_index> raw_names = cadmium::dynamic::modeling::atomic<ATOMIC, TIME, Args...>::get_input_ports();

            	return get_ports<input_ports>(raw_names);
            };

			vector<shared_ptr<cadmium::dynamic::modeling::model>> get_models() override {
				return {};
			};

			vector<cadmium::dynamic::modeling::EIC> get_eics() override {
				return {};
			};

			vector<cadmium::dynamic::modeling::EOC> get_eocs() override {
				return {};
			};

			vector<cadmium::dynamic::modeling::IC> get_ics() override {
				return {};
			};

			string messages_by_port_as_string(cadmium::dynamic::message_bags outbox) const override {
				ostringstream oss;
				web::print_dynamic_messages_by_port<output_ports>(oss, outbox);
				return oss.str();
			};
		};

		using EICs = cadmium::dynamic::modeling::EICs;
		using EOCs = cadmium::dynamic::modeling::EOCs;
		using ICs = cadmium::dynamic::modeling::ICs;
		using Models = cadmium::dynamic::modeling::Models;
		using Ports = cadmium::dynamic::modeling::Ports;

		template<typename TIME>
		class coupled_web : public cadmium::dynamic::modeling::coupled<TIME>, public web_extension{

		private:
			string _m_class = "";

		public:
			coupled_web():
				cadmium::dynamic::modeling::coupled<TIME>() { };

			coupled_web(string model_id, string m_class) :
				cadmium::dynamic::modeling::coupled<TIME>(model_id)  {
				_m_class = m_class;
			}

			coupled_web(string id, string m_class, Models models, Ports input_ports,  Ports output_ports, EICs eic, EOCs eoc, ICs ic):
				cadmium::dynamic::modeling::coupled<TIME>(id, models, input_ports, output_ports, eic, eoc, ic) {
				_m_class = m_class;
			}

			string get_id() override {
				return cadmium::dynamic::modeling::coupled<TIME>::get_id();
			};

			string get_class() {
				return _m_class;
			}

			virtual string get_type() override {
				return "coupled";
			}

			// This is super awkward, it needs an implementation but never
			// gets called. There's likely a better way to do this.
			message_type get_state_message_type() override {
				return message_type();
			};

			map<string, message_type> get_output_ports_message_types() override {
            	return map<string, message_type>();
            };

			map<string, message_type> get_input_ports_message_types() override {
            	return map<string, message_type>();
            };

			vector<shared_ptr<cadmium::dynamic::modeling::model>> get_models() override {
				return this->_models;
			};

			vector<cadmium::dynamic::modeling::EIC> get_eics() override {
				return this->_eic;
			};

			vector<cadmium::dynamic::modeling::EOC> get_eocs() override {
				return this->_eoc;
			};

			vector<cadmium::dynamic::modeling::IC> get_ics() override {
				return this->_ic;
			};
		};

		template<typename TIME>
		class top_web : public web::coupled_web<TIME> {

		private:
			string _class = "";

		public:
			top_web():
				web::coupled_web<TIME>() { };

			top_web(string _id, string _m_class) :
				web::coupled_web<TIME>(_id, _m_class)  { }

			top_web(string id, string m_class, Models models, Ports input_ports,  Ports output_ports, EICs eic, EOCs eoc, ICs ic):
				web::coupled_web<TIME>(id, m_class, models, input_ports, output_ports, eic, eoc, ic) { }

			virtual string get_type() {
				return "top";
			}
		};

		template<template <typename T> typename ATOMIC, typename TIME, typename... Args >
		shared_ptr<cadmium::web::atomic_web<ATOMIC, TIME, Args...>> make_web_atomic_model(const string& model_id, string m_class, Args&&... args) {
			return make_shared<cadmium::web::atomic_web<ATOMIC, TIME, Args...>>(model_id, m_class, forward<Args>(args)...);
		}

		template<typename TIME>
		shared_ptr<cadmium::web::coupled_web<TIME>> make_web_coupled_model(string id, string m_class, Models models, Ports i_ports, Ports o_ports, EICs eics, EOCs eocs, ICs ics) {
		    return make_shared<cadmium::web::coupled_web<TIME>>(id, m_class, models, i_ports, o_ports, eics, eocs, ics);
		}

		template<typename TIME>
		shared_ptr<cadmium::web::top_web<TIME>> make_web_top_model(string id, string m_class, Models models, Ports i_ports, Ports o_ports, EICs eics, EOCs eocs, ICs ics) {
			return make_shared<cadmium::web::top_web<TIME>>(id, m_class, models, i_ports, o_ports, eics, eocs, ics);
		}
	}
}

#endif // WEB_MODEL_HPP
