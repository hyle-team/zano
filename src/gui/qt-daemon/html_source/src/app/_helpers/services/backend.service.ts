import {Injectable} from '@angular/core';
import {Observable} from 'rxjs';
import {TranslateService} from '@ngx-translate/core';
import {VariablesService} from './variables.service';
import {ModalService} from './modal.service';
import {MoneyToIntPipe} from '../pipes/money-to-int.pipe';

@Injectable()
export class BackendService {

  backendObject: any;
  backendLoaded = false;

  constructor(private translate: TranslateService, private variablesService: VariablesService, private modalService: ModalService, private moneyToIntPipe: MoneyToIntPipe) {}

  private Debug(type, message) {
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
        break;
      case 'CORE_BUSY':
        if (command !== 'get_all_aliases') {
          error_translate = 'ERRORS.CORE_BUSY';
        }
        break;
      case 'OVERFLOW':
        if (command !== 'get_all_aliases') {
          error_translate = '';
        }
        break;
      case 'INTERNAL_ERROR:daemon is busy':
        error_translate = 'ERRORS.DAEMON_BUSY';
        break;
      case 'INTERNAL_ERROR:not enough money':
      case 'INTERNAL_ERROR:NOT_ENOUGH_MONEY':
        if (command === 'cancel_offer') {
          error_translate = this.translate.instant('ERRORS.NO_MONEY_REMOVE_OFFER', {
            'fee': '0.01',
            'currency': 'ZAN'
          });
        } else {
          error_translate = 'INFORMER.NO_MONEY';
        }
        break;
      case 'INTERNAL_ERROR:not enough outputs to mix':
        error_translate = 'ERRORS.NOT_ENOUGH_OUTPUTS_TO_MIX';
        break;
      case 'INTERNAL_ERROR:transaction is too big':
        error_translate = 'ERRORS.TRANSACTION_IS_TO_BIG';
        break;
      case 'INTERNAL_ERROR:Transfer attempt while daemon offline':
        error_translate = 'ERRORS.TRANSFER_ATTEMPT';
        break;
      case 'ACCESS_DENIED':
        error_translate = 'ERRORS.ACCESS_DENIED';
        break;
      case 'INTERNAL_ERROR:transaction was rejected by daemon':
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
      case 'WRONG_PASSWORD':
      case 'WRONG_PASSWORD:invalid password':
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
      default:
        error_translate = error;
    }
    if (error.indexOf('FAIL:failed to save file') > -1) {
      error_translate = 'ERRORS.FILE_NOT_SAVED';
    }
    if (error_translate !== '') {
      this.modalService.prepareModal('error', error_translate);
    }
  }


  private commandDebug(command, params, result) {
    this.Debug(2, '----------------- ' + command + ' -----------------');
    const debug = {
      _send_params: params,
      _result: result
    };
    this.Debug(2, debug);
    try {
      this.Debug(2, JSON.parse(result));
    } catch (e) {
      this.Debug(2, {response_data: result, error_code: 'OK'});
    }
  }

  private asVal(data) {
    return {v: data};
  }

  private backendCallback(resultStr, params, callback, command) {
    let Result = resultStr;
    if (command !== 'get_clipboard') {
      if (!resultStr || resultStr === '') {
        Result = {};
      } else {
        try {
          Result = JSON.parse(resultStr);
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

    const Status = (Result.error_code === 'OK' || Result.error_code === 'TRUE');

    if (!Status && Status !== undefined && Result.error_code !== undefined) {
      this.Debug(1, 'API error for command: "' + command + '". Error code: ' + Result.error_code);
    }
    const data = ((typeof Result === 'object') && 'response_data' in Result) ? Result.response_data : Result;

    let res_error_code = false;
    if (typeof Result === 'object' && 'error_code' in Result && Result.error_code !== 'OK' && Result.error_code !== 'TRUE' && Result.error_code !== 'FALSE') {
      this.informerRun(Result.error_code, params, command);
      res_error_code = Result.error_code;
    }

    // if ( command === 'get_offers_ex' ){
    //   Service.printLog( "get_offers_ex offers count "+((data.offers)?data.offers.length:0) );
    // }

    if (typeof callback === 'function') {
      callback(Status, data, res_error_code);
    } else {
      return data;
    }
  }


  private runCommand(command, params?, callback?) {
    if (this.backendObject) {
      const Action = this.backendObject[command];
      if (!Action) {
        this.Debug(0, 'Run Command Error! Command "' + command + '" don\'t found in backendObject');
      } else {
        const that = this;
        params = (typeof params === 'string') ? params : JSON.stringify(params);
        if (params === undefined || params === '{}') {
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
        callback(JSON.parse(str));
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
    this.runCommand('store_app_data', this.variablesService.settings, callback);
  }

  getSecureAppData(pass, callback) {
    this.runCommand('get_secure_app_data', pass, callback);
  }

  storeSecureAppData(callback) {
    if (this.variablesService.appPass === '') {
      return callback(false);
    }
    const wallets = [];
    this.variablesService.wallets.forEach((wallet) => {
      wallets.push({name: wallet.name, pass: wallet.pass, path: wallet.path});
    });
    this.backendObject['store_secure_app_data'](JSON.stringify(wallets), this.variablesService.appPass, (dataStore) => {
      this.backendCallback(dataStore, {}, callback, 'store_secure_app_data');
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

  generateWallet(path, pass, callback) {
    const params = {
      path: path,
      pass: pass
    };
    this.runCommand('generate_wallet', params, callback);
  }

  openWallet(path, pass, testEmpty, callback) {
    const params = {
      path: path,
      pass: pass
    };
    params['testEmpty'] = !!(testEmpty);
    this.runCommand('open_wallet', params, callback);
  }

  closeWallet(wallet_id, callback) {
    this.runCommand('close_wallet', {wallet_id: wallet_id}, callback);
  }

  getSmartSafeInfo(wallet_id, callback) {
    this.runCommand('get_smart_safe_info', {wallet_id: +wallet_id}, callback);
  }

  runWallet(wallet_id, callback) {
    this.runCommand('run_wallet', {wallet_id: +wallet_id}, callback);
  }

  isValidRestoreWalletText(text, callback) {
    this.runCommand('is_valid_restore_wallet_text', text, callback);
  }

  restoreWallet(path, pass, restore_key, callback) {
    const params = {
      restore_key: restore_key,
      path: path,
      pass: pass
    };
    this.runCommand('restore_wallet', params, callback);
  }


  sendMoney(from_wallet_id, to_address, amount, fee, mixin, comment, callback) {
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
      push_payer: true
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
        a_pledge: this.moneyToIntPipe.transform(a_pledge) - this.moneyToIntPipe.transform(to_pay),
        b_pledge: this.moneyToIntPipe.transform(b_pledge)
      },
      payment_id: payment_id,
      expiration_period: parseInt(time, 10) * 60 * 60,
      fee: this.moneyToIntPipe.transform('0.01'),
      b_fee: this.moneyToIntPipe.transform('0.01')
    };
    this.Debug(1, params);
    this.runCommand('create_proposal', params, callback);
  }

  getContracts(wallet_id, callback) {
    const params = {
      wallet_id: parseInt(wallet_id, 10)
    };
    this.Debug(1, params);
    this.runCommand('get_contracts', params, callback);
  }

  acceptProposal(wallet_id, contract_id, callback) {
    const params = {
      wallet_id: parseInt(wallet_id, 10),
      contract_id: contract_id
    };
    this.Debug(1, params);
    this.runCommand('accept_proposal', params, callback);
  }

  releaseProposal(wallet_id, contract_id, release_type, callback) {
    const params = {
      wallet_id: parseInt(wallet_id, 10),
      contract_id: contract_id,
      release_type: release_type // "normal" or "burn"
    };
    this.Debug(1, params);
    this.runCommand('release_contract', params, callback);
  }

  requestCancelContract(wallet_id, contract_id, time, callback) {
    const params = {
      wallet_id: parseInt(wallet_id, 10),
      contract_id: contract_id,
      fee: this.moneyToIntPipe.transform('0.01'),
      expiration_period: parseInt(time, 10) * 60 * 60
    };
    this.Debug(1, params);
    this.runCommand('request_cancel_contract', params, callback);
  }

  acceptCancelContract(wallet_id, contract_id, callback) {
    const params = {
      wallet_id: parseInt(wallet_id, 10),
      contract_id: contract_id
    };
    this.Debug(1, params);
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

  is_remnotenode_mode_preconfigured(callback) {
    this.runCommand('is_remnotenode_mode_preconfigured', {}, callback);
  }

  start_backend(node, host, port, callback) {
    const params = {
      configure_for_remote_node: node,
      remote_node_host: host,
      remote_node_port: parseInt(port, 10)
    };
    this.runCommand('start_backend', params, callback);
  }

}


/*

    var deferred = null;

    var Service = {







      /!*  API  *!/

      toggleAutoStart: function (value) {
        return this.runCommand('toggle_autostart', asVal(value));
      },



      getDefaultFee: function (callback) {
        return this.runCommand('get_default_fee', {}, callback);
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



      setLogLevel: function (level) {
        return this.runCommand('set_log_level', asVal(level))
      },

      resetWalletPass: function (wallet_id, pass, callback) {
        this.runCommand('reset_wallet_password', {wallet_id: wallet_id, pass: pass}, callback);
      },

      getVersion: function (callback) {
        this.runCommand('get_version', {}, function (status, version) {
          callback(version)
        })
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





      registerAlias: function (wallet_id, alias, address, fee, comment, reward, callback) {
        var params = {
          "wallet_id": wallet_id,
          "alias": {
            "alias": alias,
            "address": address,
            "tracking_key": "",
            "comment": comment
          },
          "fee": $filter('money_to_int')(fee),
          "reward": $filter('money_to_int')(reward)
        };
        this.runCommand('request_alias_registration', params, callback);
      },

      updateAlias: function (wallet_id, alias, fee, callback) {
        var params = {
          wallet_id: wallet_id,
          alias: {
            "alias": alias.name.replace("@", ""),
            "address": alias.address,
            "tracking_key": "",
            "comment": alias.comment
          },
          fee: $filter('money_to_int')(fee)
        };
        this.runCommand('request_alias_update', params, callback);
      },

      getAllAliases: function (callback) {
        this.runCommand('get_all_aliases', {}, callback);
      },

      getAliasByName: function (value, callback) {
        return this.runCommand('get_alias_info_by_name', value, callback);
      },

      getAliasByAddress: function (value, callback) {
        return this.runCommand('get_alias_info_by_address', value, callback);
      },

      getPoolInfo: function (callback) {
        this.runCommand('get_tx_pool_info', {}, callback);
      },

      localization: function (stringsArray, title, callback) {
        var data = {
          strings: stringsArray,
          language_title: title
        };
        this.runCommand('set_localization_strings', data, callback);
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


      getAliasCoast: function (alias, callback) {
        this.runCommand('get_alias_coast', asVal(alias), callback);
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





      /!*  API END  *!/

    };
    return Service;
  }]);

})();

*/

