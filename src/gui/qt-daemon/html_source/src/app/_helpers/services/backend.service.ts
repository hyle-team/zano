import {Injectable} from '@angular/core';
import {Observable} from 'rxjs';
import {TranslateService} from '@ngx-translate/core';
import {VariablesService} from './variables.service';
import {ModalService} from './modal.service';
import {MoneyToIntPipe} from '../pipes/money-to-int.pipe';
import JSONBigNumber from 'json-bignumber';
import {BigNumber} from 'bignumber.js';

@Injectable()
export class BackendService {

  backendObject: any;
  backendLoaded = false;

  constructor(
    private translate: TranslateService,
    private variablesService: VariablesService,
    private modalService: ModalService,
    private moneyToIntPipe: MoneyToIntPipe
  ) {
  }

  static bigNumberParser(key, val) {
    if (val.constructor.name === 'BigNumber' && ['balance', 'unlocked_balance', 'amount', 'fee', 'b_fee', 'to_pay', 'a_pledge', 'b_pledge', 'coast', 'a'].indexOf(key) === -1) {
      return val.toNumber();
    }
    if (key === 'rcv' || key === 'spn') {
      for (let i = 0; i < val.length; i++) {
        val[i] = new BigNumber(val[i]);
      }
    }
    return val;
  }

  static Debug(type, message) {
    switch (type) {
      case 0:
        console.error(message);
        break;
      case 1:
        console.warn(message);
        break;
      case 2:
        console.log(message);
        break;
      default:
        console.log(message);
        break;
    }
  }

  private informerRun(error, params, command) {
    let error_translate = '';

    switch (error) {
      case 'NOT_ENOUGH_MONEY':
        error_translate = 'ERRORS.NOT_ENOUGH_MONEY';
        // error_translate = 'ERRORS.NO_MONEY'; maybe that one?
        if (command === 'cancel_offer') {
          error_translate = this.translate.instant('ERRORS.NO_MONEY_REMOVE_OFFER', {
            'fee': this.variablesService.default_fee,
            'currency': this.variablesService.defaultCurrency
          });
        }
        break;
      case 'CORE_BUSY':
        error_translate = 'ERRORS.CORE_BUSY';
        break;
      case 'BUSY':
        error_translate = 'ERRORS.DAEMON_BUSY';
        break;
      case 'OVERFLOW':
        if (command !== 'get_all_aliases') {
          error_translate = '';
        }
        break;
      case 'NOT_ENOUGH_OUTPUTS_FOR_MIXING':
        error_translate = 'ERRORS.NOT_ENOUGH_OUTPUTS_TO_MIX';
        break;
      case 'TX_IS_TOO_BIG':
        error_translate = 'ERRORS.TRANSACTION_IS_TO_BIG';
        break;
      case 'DISCONNECTED':
        error_translate = 'ERRORS.TRANSFER_ATTEMPT';
        break;
      case 'ACCESS_DENIED':
        error_translate = 'ERRORS.ACCESS_DENIED';
        break;
      case 'TX_REJECTED':
        // if (command === 'request_alias_registration') {
        // error_translate = 'INFORMER.ALIAS_IN_REGISTER';
        // } else {
        error_translate = 'ERRORS.TRANSACTION_ERROR';
        // }
        break;
      case 'INTERNAL_ERROR':
        error_translate = 'ERRORS.TRANSACTION_ERROR';
        break;
      case 'BAD_ARG':
        error_translate = 'ERRORS.BAD_ARG';
        break;
      case 'WALLET_WRONG_ID':
        error_translate = 'ERRORS.WALLET_WRONG_ID';
        break;
      case 'WALLET_WATCH_ONLY_NOT_SUPPORTED':
        error_translate = 'ERRORS.WALLET_WATCH_ONLY_NOT_SUPPORTED';
        break;
      case 'WRONG_PASSWORD':
        params = JSON.parse(params);
        if (!params.testEmpty) {
          error_translate = 'ERRORS.WRONG_PASSWORD';
        }
        break;
      case 'FILE_RESTORED':
        if (command === 'open_wallet') {
          error_translate = 'ERRORS.FILE_RESTORED';
        }
        break;
      case 'FILE_NOT_FOUND':
        if (command !== 'open_wallet' && command !== 'get_alias_info_by_name' && command !== 'get_alias_info_by_address') {
          error_translate = this.translate.instant('ERRORS.FILE_NOT_FOUND');
          params = JSON.parse(params);
          if (params.path) {
            error_translate += ': ' + params.path;
          }
        }
        break;
      case 'NOT_FOUND':
        if (command !== 'open_wallet' && command !== 'get_alias_info_by_name' && command !== 'get_alias_info_by_address') {
          error_translate = this.translate.instant('ERRORS.FILE_NOT_FOUND');
          params = JSON.parse(params);
          if (params.path) {
            error_translate += ': ' + params.path;
          }
        }
        break;
      case 'CANCELED':
      case '':
        break;
      case 'FAIL':
        if (command === 'create_proposal' || command === 'accept_proposal' || command === 'release_contract' || command === 'request_cancel_contract' || command === 'accept_cancel_contract') {
          error_translate = ' ';
        }
        break;
      case 'ALREADY_EXISTS':
        error_translate = 'ERRORS.FILE_EXIST';
        break;
      case 'FAILED':
        BackendService.Debug(0, `Error: (${error}) was triggered by command: ${command}`);
        break;
      default:
        error_translate = error;
    }
    if (error.indexOf('FAIL:failed to save file') > -1) {
      error_translate = 'ERRORS.FILE_NOT_SAVED';
    }
    if (error.indexOf('FAILED:failed to open binary wallet file for saving') > -1 && command === 'generate_wallet') {
      error_translate = '';
    }

    if (error_translate !== '') {
      this.modalService.prepareModal('error', error_translate);
    }
  }


  private commandDebug(command, params, result) {
    BackendService.Debug(2, '----------------- ' + command + ' -----------------');
    const debug = {
      _send_params: params,
      _result: result
    };
    BackendService.Debug(2, debug);
    try {
      BackendService.Debug(2, JSONBigNumber.parse(result, BackendService.bigNumberParser));
    } catch (e) {
      BackendService.Debug(2, {response_data: result, error_code: 'OK'});
    }
  }

  private backendCallback(resultStr, params, callback, command) {
    let Result = resultStr;
    if (command !== 'get_clipboard') {
      if (!resultStr || resultStr === '') {
        Result = {};
      } else {
        try {
          Result = JSONBigNumber.parse(resultStr, BackendService.bigNumberParser);
        } catch (e) {
          Result = {response_data: resultStr, error_code: 'OK'};
        }
      }
    } else {
      Result = {
        error_code: 'OK',
        response_data: Result
      };
    }

    const core_busy = Result.error_code === 'CORE_BUSY';
    const Status = (Result.error_code === 'OK' || Result.error_code === 'TRUE');

    if (!Status && Status !== undefined && Result.error_code !== undefined) {
      BackendService.Debug(1, 'API error for command: "' + command + '". Error code: ' + Result.error_code);
    }
    const data = ((typeof Result === 'object') && 'response_data' in Result) ? Result.response_data : Result;

    let res_error_code = false;
    if (typeof Result === 'object' && 'error_code' in Result && Result.error_code !== 'OK' && Result.error_code !== 'TRUE' && Result.error_code !== 'FALSE') {
      if (core_busy) {
        setTimeout( () => {
          // this is will avoid update data when user
          // on other wallet after CORE_BUSY (blink of data)
          if (command !== 'get_recent_transfers') {
            this.runCommand(command, params, callback);
          } else {
            const current_wallet_id = this.variablesService.currentWallet.wallet_id;
            if (current_wallet_id === params.wallet_id) {
              this.runCommand(command, params, callback);
            }
          }
        }, 50);
      } else {
        this.informerRun(Result.error_code, params, command);
        res_error_code = Result.error_code;
      }
    }

    // if ( command === 'get_offers_ex' ){
    //   Service.printLog( "get_offers_ex offers count "+((data.offers)?data.offers.length:0) );
    // }

    if (!core_busy) {
      if (typeof callback === 'function') {
        callback(Status, data, res_error_code);
      } else {
        return data;
      }
    }
  }


  private runCommand(command, params?, callback?) {
    if (this.backendObject) {
      if (command === 'get_recent_transfers') {
        this.variablesService.get_recent_transfers = true;
      }
      const Action = this.backendObject[command];
      if (!Action) {
        BackendService.Debug(0, 'Run Command Error! Command "' + command + '" don\'t found in backendObject');
      } else {
        const that = this;
        params = (typeof params === 'string') ? params : JSONBigNumber.stringify(params);
        if (params === undefined || params === '{}') {
          if (command === 'get_recent_transfers') {
            this.variablesService.get_recent_transfers = false;
          }
          Action(function (resultStr) {
            that.commandDebug(command, params, resultStr);
            return that.backendCallback(resultStr, params, callback, command);
          });
        } else {
          Action(params, function (resultStr) {
            that.commandDebug(command, params, resultStr);
            return that.backendCallback(resultStr, params, callback, command);
          });
        }
      }
    }
  }


  eventSubscribe(command, callback) {
    if (command === 'on_core_event') {
      this.backendObject[command].connect(callback);
    } else {
      this.backendObject[command].connect((str) => {
        callback(JSONBigNumber.parse(str, BackendService.bigNumberParser));
      });
    }
  }


  initService() {
    return new Observable(
      observer => {
        if (!this.backendLoaded) {
          this.backendLoaded = true;
          const that = this;
          (<any>window).QWebChannel((<any>window).qt.webChannelTransport, function (channel) {
            that.backendObject = channel.objects.mediator_object;
            observer.next('ok');
          });
        } else {
          if (!this.backendObject) {
            observer.error('error');
            observer.error('error');
          }
        }
      }
    );
  }


  webkitLaunchedScript() {
    return this.runCommand('webkit_launched_script');
  }

  quitRequest() {
    return this.runCommand('on_request_quit');
  }

  getAppData(callback) {
    this.runCommand('get_app_data', {}, callback);
  }

  storeAppData(callback?) {
    if (this.variablesService.wallets.length) {
      this.variablesService.settings.wallets = [];
      this.variablesService.wallets.forEach((wallet) => {
        this.variablesService.settings.wallets.push({name: wallet.name, path: wallet.path});
      });
    }
    this.runCommand('store_app_data', this.variablesService.settings, callback);
  }

  getSecureAppData(pass, callback) {
    this.runCommand('get_secure_app_data', pass, callback);
  }

  setMasterPassword(pass, callback) {
    this.runCommand('set_master_password', pass, callback);
  }

  checkMasterPassword(pass, callback) {
    this.runCommand('check_master_password', pass, callback);
  }
  storeSecureAppData(callback?) {
    let data;
    const wallets = [];
    const contacts = [];
    this.variablesService.wallets.forEach((wallet) => {
      wallets.push({name: wallet.name, pass: wallet.pass, path: wallet.path, staking: wallet.staking});
    });
    this.variablesService.contacts.forEach((contact) => {
      contacts.push({name: contact.name, address: contact.address, notes: contact.notes});
    });
    data = {wallets: wallets, contacts: contacts};
    this.backendObject['store_secure_app_data'](JSON.stringify(data), this.variablesService.appPass, (dataStore) => {
      this.backendCallback(dataStore, {}, callback, 'store_secure_app_data');
    });
  }

  dropSecureAppData(callback?) {
    this.backendObject['drop_secure_app_data']((dataStore) => {
      this.backendCallback(dataStore, {}, callback, 'drop_secure_app_data');
    });
  }

  haveSecureAppData(callback) {
    this.runCommand('have_secure_app_data', {}, callback);
  }

  saveFileDialog(caption, fileMask, default_path, callback) {
    const dir = default_path ? default_path : '/';
    const params = {
      caption: caption,
      filemask: fileMask,
      default_dir: dir
    };
    this.runCommand('show_savefile_dialog', params, callback);
  }

  openFileDialog(caption, fileMask, default_path, callback) {
    const dir = default_path ? default_path : '/';
    const params = {
      caption: caption,
      filemask: fileMask,
      default_dir: dir
    };
    this.runCommand('show_openfile_dialog', params, callback);
  }

  storeFile(path, buff) {
    this.backendObject['store_to_file'](path, buff);
  }

  loadFile(path, callback) {
    this.runCommand('load_from_file', path, callback);
  }

  generateWallet(path, pass, callback) {
    const params = {
      path: path,
      pass: pass
    };
    this.runCommand('generate_wallet', params, callback);
  }

  openWallet(path, pass, txs_to_return, testEmpty, callback) {
    const params = {
      path: path,
      pass: pass,
      txs_to_return: txs_to_return
    };
    params['testEmpty'] = !!(testEmpty);
    this.runCommand('open_wallet', params, callback);
  }

  closeWallet(wallet_id, callback?) {
    this.runCommand('close_wallet', {wallet_id: +wallet_id}, callback);
  }

  getSmartWalletInfo({wallet_id, seed_password}, callback) {
    this.runCommand('get_smart_wallet_info', {wallet_id: +wallet_id, seed_password}, callback);
  }

  getSeedPhraseInfo(param, callback) {
    this.runCommand('get_seed_phrase_info', param, callback);
  }

  runWallet(wallet_id, callback?) {
    this.runCommand('run_wallet', {wallet_id: +wallet_id}, callback);
  }

  isValidRestoreWalletText(param, callback) {
    this.runCommand('is_valid_restore_wallet_text', param, callback);
  }

  restoreWallet(path, pass, seed_phrase, seed_pass, callback) {
    const params = {
      seed_phrase: seed_phrase,
      path: path,
      pass: pass,
      seed_pass
    };
    this.runCommand('restore_wallet', params, callback);
  }

  sendMoney(from_wallet_id, to_address, amount, fee, mixin, comment, hide, callback) {
    const params = {
      wallet_id: parseInt(from_wallet_id, 10),
      destinations: [
        {
          address: to_address,
          amount: amount
        }
      ],
      mixin_count: (mixin) ? parseInt(mixin, 10) : 0,
      lock_time: 0,
      fee: this.moneyToIntPipe.transform(fee),
      comment: comment,
      push_payer: !hide
    };
    this.runCommand('transfer', params, callback);
  }

  validateAddress(address, callback) {
    this.runCommand('validate_address', address, callback);
  }

  setClipboard(str, callback?) {
    return this.runCommand('set_clipboard', str, callback);
  }

  getClipboard(callback) {
    return this.runCommand('get_clipboard', {}, callback);
  }

  createProposal(wallet_id, title, comment, a_addr, b_addr, to_pay, a_pledge, b_pledge, time, payment_id, callback) {
    const params = {
      wallet_id: parseInt(wallet_id, 10),
      details: {
        t: title,
        c: comment,
        a_addr: a_addr,
        b_addr: b_addr,
        to_pay: this.moneyToIntPipe.transform(to_pay),
        a_pledge: this.moneyToIntPipe.transform(a_pledge),
        b_pledge: this.moneyToIntPipe.transform(b_pledge)
      },
      payment_id: payment_id,
      expiration_period: parseInt(time, 10) * 60 * 60,
      fee: this.variablesService.default_fee_big,
      b_fee: this.variablesService.default_fee_big
    };
    BackendService.Debug(1, params);
    this.runCommand('create_proposal', params, callback);
  }

  getContracts(wallet_id, callback) {
    const params = {
      wallet_id: parseInt(wallet_id, 10)
    };
    BackendService.Debug(1, params);
    this.runCommand('get_contracts', params, callback);
  }

  acceptProposal(wallet_id, contract_id, callback) {
    const params = {
      wallet_id: parseInt(wallet_id, 10),
      contract_id: contract_id
    };
    BackendService.Debug(1, params);
    this.runCommand('accept_proposal', params, callback);
  }

  releaseProposal(wallet_id, contract_id, release_type, callback) {
    const params = {
      wallet_id: parseInt(wallet_id, 10),
      contract_id: contract_id,
      release_type: release_type // "normal" or "burn"
    };
    BackendService.Debug(1, params);
    this.runCommand('release_contract', params, callback);
  }

  requestCancelContract(wallet_id, contract_id, time, callback) {
    const params = {
      wallet_id: parseInt(wallet_id, 10),
      contract_id: contract_id,
      fee: this.variablesService.default_fee_big,
      expiration_period: parseInt(time, 10) * 60 * 60
    };
    BackendService.Debug(1, params);
    this.runCommand('request_cancel_contract', params, callback);
  }

  acceptCancelContract(wallet_id, contract_id, callback) {
    const params = {
      wallet_id: parseInt(wallet_id, 10),
      contract_id: contract_id
    };
    BackendService.Debug(1, params);
    this.runCommand('accept_cancel_contract', params, callback);
  }

  getMiningHistory(wallet_id, callback) {
    this.runCommand('get_mining_history', {wallet_id: parseInt(wallet_id, 10)}, callback);
  }

  startPosMining(wallet_id, callback?) {
    this.runCommand('start_pos_mining', {wallet_id: parseInt(wallet_id, 10)}, callback);
  }

  stopPosMining(wallet_id, callback?) {
    this.runCommand('stop_pos_mining', {wallet_id: parseInt(wallet_id, 10)}, callback);
  }

  openUrlInBrowser(url, callback?) {
    this.runCommand('open_url_in_browser', url, callback);
  }

  start_backend(node, host, port, callback) {
    const params = {
      configure_for_remote_node: node,
      remote_node_host: host,
      remote_node_port: parseInt(port, 10)
    };
    this.runCommand('start_backend', params, callback);
  }

  getDefaultFee(callback) {
    this.runCommand('get_default_fee', {}, callback);
  }

  setBackendLocalization(stringsArray, title, callback?) {
    const params = {
      strings: stringsArray,
      language_title: title
    };
    this.runCommand('set_localization_strings', params, callback);
  }

  registerAlias(wallet_id, alias, address, fee, comment, reward, callback) {
    const params = {
      wallet_id: wallet_id,
      alias: {
        alias: alias,
        address: address,
        tracking_key: '',
        comment: comment
      },
      fee: this.moneyToIntPipe.transform(fee),
      reward: this.moneyToIntPipe.transform(reward)
    };
    this.runCommand('request_alias_registration', params, callback);
  }

  updateAlias(wallet_id, alias, fee, callback) {
    const params = {
      wallet_id: wallet_id,
      alias: {
        alias: alias.name.replace('@', ''),
        address: alias.address,
        tracking_key: '',
        comment: alias.comment
      },
      fee: this.moneyToIntPipe.transform(fee)
    };
    this.runCommand('request_alias_update', params, callback);
  }

  getAllAliases(callback) {
    this.runCommand('get_all_aliases', {}, callback);
  }

  getAliasByName(value, callback) {
    return this.runCommand('get_alias_info_by_name', value, callback);
  }

  getAliasByAddress(value, callback) {
    return this.runCommand('get_alias_info_by_address', value, callback);
  }

  getAliasCoast(alias, callback) {
    this.runCommand('get_alias_coast', {v: alias}, callback);
  }

  getWalletAlias(address) {
    if (address !== null && this.variablesService.daemon_state === 2) {
      if (this.variablesService.aliasesChecked[address] == null) {
        this.variablesService.aliasesChecked[address] = {};
        if (this.variablesService.aliases.length) {
          for (let i = 0, length = this.variablesService.aliases.length; i < length; i++) {
            if (i in this.variablesService.aliases && this.variablesService.aliases[i]['address'] === address) {
              this.variablesService.aliasesChecked[address]['name'] = this.variablesService.aliases[i].name;
              this.variablesService.aliasesChecked[address]['address'] = this.variablesService.aliases[i].address;
              this.variablesService.aliasesChecked[address]['comment'] = this.variablesService.aliases[i].comment;
              return this.variablesService.aliasesChecked[address];
            }
          }
        }
        this.getAliasByAddress(address, (status, data) => {
          if (status) {
            this.variablesService.aliasesChecked[data.address]['name'] = '@' + data.alias;
            this.variablesService.aliasesChecked[data.address]['address'] = data.address;
            this.variablesService.aliasesChecked[data.address]['comment'] = data.comment;
          }
        });
      }
      return this.variablesService.aliasesChecked[address];
    }
    return {};
  }

  getContactAlias() {
    if (this.variablesService.contacts.length && this.variablesService.daemon_state === 2) {
      this.variablesService.contacts.map(contact => {
        this.getAliasByAddress(contact.address, (status, data) => {
          if (status) {
            if (data.alias) {
              contact.alias = '@' + data.alias;
            }
          } else {
            contact.alias = null;
          }
        });
      });
    }
  }

  getRecentTransfers( id, offset, count, exclude_mining_txs, callback) {
    const params = {
      wallet_id: id,
      offset: offset,
      count: count,
      exclude_mining_txs: exclude_mining_txs
    };
      this.runCommand('get_recent_transfers', params, callback);
  }

  getPoolInfo(callback) {
    this.runCommand('get_tx_pool_info', {}, callback);
  }

  getVersion(callback) {
    this.runCommand('get_version', {}, (status, version) => {
      this.runCommand('get_network_type', {}, (status, type) => {
        callback(version, type);
      });
    });
  }

  setLogLevel(level) {
    return this.runCommand('set_log_level', {v: level});
  }

}


/*

      toggleAutoStart: function (value) {
        return this.runCommand('toggle_autostart', asVal(value));
      },

      getOptions: function (callback) {
        return this.runCommand('get_options', {}, callback);
      },

      isFileExist: function (path, callback) {
        return this.runCommand('is_file_exist', path, callback);
      },

      isAutoStartEnabled: function (callback) {
        this.runCommand('is_autostart_enabled', {}, function (status, data) {
          if (angular.isFunction(callback)) {
            callback('error_code' in data && data.error_code !== 'FALSE')
          }
        });
      },

      resetWalletPass: function (wallet_id, pass, callback) {
        this.runCommand('reset_wallet_password', {wallet_id: wallet_id, pass: pass}, callback);
      },



      getOsVersion: function (callback) {
        this.runCommand('get_os_version', {}, function (status, version) {
          callback(version)
        })
      },

      getLogFile: function (callback) {
        this.runCommand('get_log_file', {}, function (status, version) {
          callback(version)
        })
      },

      resync_wallet: function (wallet_id, callback) {
        this.runCommand('resync_wallet', {wallet_id: wallet_id}, callback);
      },

      storeFile: function (path, buff, callback) {
        this.backendObject['store_to_file'](path, (typeof buff === 'string' ? buff : JSON.stringify(buff)), function (data) {
          backendCallback(data, {}, callback, 'store_to_file');
        });
      },

      getMiningEstimate: function (amount_coins, time, callback) {
        var params = {
          "amount_coins": $filter('money_to_int')(amount_coins),
          "time": parseInt(time)
        };
        this.runCommand('get_mining_estimate', params, callback);
      },

      backupWalletKeys: function (wallet_id, path, callback) {
        var params = {
          "wallet_id": wallet_id,
          "path": path
        };
        this.runCommand('backup_wallet_keys', params, callback);
      },

      setBlockedIcon: function (enabled, callback) {
        var mode = (enabled) ? "blocked" : "normal";
        Service.runCommand('bool_toggle_icon', mode, callback);
      },

      getWalletInfo: function (wallet_id, callback) {
        this.runCommand('get_wallet_info', {wallet_id: wallet_id}, callback);
      },

      printText: function (content) {
        return this.runCommand('print_text', {html_text: content});
      },

      printLog: function (msg, log_level) {
        return this.runCommand('print_log', {msg: msg, log_level: log_level});
      },

*/

