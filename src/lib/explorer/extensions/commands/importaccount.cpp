/**
 * Copyright (c) 2016-2017 mvs developers 
 *
 * This file is part of metaverse-explorer.
 *
 * metaverse-explorer is free software: you can redistribute it and/or
 * modify it under the terms of the GNU Affero General Public License with
 * additional permissions to the one published by the Free Software
 * Foundation, either version 3 of the License, or (at your option)
 * any later version. For more information see LICENSE.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include <metaverse/explorer/json_helper.hpp>
#include <metaverse/explorer/dispatch.hpp>
#include <metaverse/explorer/extensions/commands/importaccount.hpp>
#include <metaverse/explorer/extensions/command_extension_func.hpp>
#include <metaverse/explorer/extensions/command_assistant.hpp>
#include <metaverse/explorer/extensions/exception.hpp> 
#include <metaverse/explorer/commands/offline_commands_impl.hpp> 

namespace libbitcoin {
namespace explorer {
namespace commands {
using namespace bc::explorer::config;

console_result importaccount::invoke (Json::Value& jv_output,
         libbitcoin::server::server_node& node)
{
    
    // parameter account name check
    auto& blockchain = node.chain_impl();
    if (blockchain.is_account_exist(auth_.name))
     throw account_existed_exception{"account already exist"};

#ifdef NDEBUG
	if (auth_.name.length() > 128 || auth_.name.length() < 3 ||
	    option_.passwd.length() > 128 || option_.passwd.length() < 6)
	    throw argument_exceed_limit_exception{"name length in [3, 128], password length in [6, 128]"};
#endif

    // are vliad mnemonic words.
    auto&& seed = get_mnemonic_to_seed(option_.language, argument_.words);
    // is vliad seed.
    auto&& hd_pri_key = get_hd_new(seed);

    auto&& mnemonic = bc::join(argument_.words);

    // create account
    auto acc = std::make_shared<bc::chain::account>();
    acc->set_name(auth_.name);
    acc->set_passwd(option_.passwd);
    acc->set_mnemonic(mnemonic, option_.passwd);
    //acc->set_hd_index(option_.hd_index); // hd_index updated in getnewaddress
    
    // flush to db
    blockchain.store_account(acc);

    // generate all account address
    auto& root = jv_output;
    root["name"] = auth_.name;
    root["mnemonic"] = mnemonic;
    if (get_api_version()) {
        root["hd_index"] += option_.hd_index;
    } else {
        root["hd_index"] = option_.hd_index;
    }
    
    uint32_t idx = 0;
    auto&& str_idx = std::to_string(option_.hd_index);
    const char* cmds2[]{"getnewaddress", auth_.name.c_str(), option_.passwd.c_str(), "-i", str_idx.c_str()};
    Json::Value addresses;
    std::stringstream sout("");
    
    for( idx = 0; idx < option_.hd_index; idx++ ) {
        sout.str("");
        if (dispatch_command(5, cmds2, addresses, node, 2) != console_result::okay) {
            throw address_generate_exception(sout.str());
        }
         
    }

    root.append(addresses);
    
    return console_result::okay;
}

} // namespace commands
} // namespace explorer
} // namespace libbitcoin

