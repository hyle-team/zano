import {Component, OnInit, NgZone, Renderer2, OnDestroy} from '@angular/core';
import {HttpClient} from '@angular/common/http';
import {TranslateService} from '@ngx-translate/core';
import {BackendService} from './_helpers/services/backend.service';
import {Router} from '@angular/router';
import {VariablesService} from './_helpers/services/variables.service';

@Component({
  selector: 'app-root',
  templateUrl: './app.component.html',
  styleUrls: ['./app.component.scss']
})
export class AppComponent implements OnInit, OnDestroy {

  intervalUpdateContractsState;
  onQuitRequest = false;

  constructor(
    private http: HttpClient,
    private renderer: Renderer2,
    public translate: TranslateService,
    private backend: BackendService,
    private router: Router,
    private variablesService: VariablesService,
    private ngZone: NgZone
  ) {
    translate.addLangs(['en', 'fr']);
    translate.setDefaultLang('en');
    // const browserLang = translate.getBrowserLang();
    // translate.use(browserLang.match(/en|fr/) ? browserLang : 'en');
  }

  ngOnInit() {
    this.backend.initService().subscribe(initMessage => {
      console.log('Init message: ', initMessage);

      this.backend.webkitLaunchedScript();


      this.backend.is_remnotenode_mode_preconfigured((status, data) => {
        // if (data === 'FALSE') {
        // } else {
        // }
        this.backend.start_backend(false, '127.0.0.1', 11512, (st2, dd2) => {
          console.log(st2, dd2);
        });
      });

      this.backend.eventSubscribe('quit_requested', () => {
        if (!this.onQuitRequest) {
          this.ngZone.run(() => {
            this.router.navigate(['/']);
          });
          this.backend.storeSecureAppData(() => {
            this.backend.storeAppData(() => {
              const recursionCloseWallets = () => {
                if (this.variablesService.wallets.length) {
                  const lastIndex = this.variablesService.wallets.length - 1;
                  this.backend.closeWallet(this.variablesService.wallets[lastIndex].wallet_id, () => {
                    this.variablesService.wallets.splice(lastIndex, 1);
                    recursionCloseWallets();
                  });
                } else {
                  this.backend.quitRequest();
                }
              };
              recursionCloseWallets();
            });
          });
        }
        this.onQuitRequest = true;
      });


      this.backend.eventSubscribe('update_wallet_status', (data) => {
        console.log('----------------- update_wallet_status -----------------');
        console.log(data);

        const wallet_state = data.wallet_state;
        const is_mining = data.is_mining;
        const wallet = this.variablesService.getWallet(data.wallet_id);

        // 1-synch, 2-ready, 3 - error
        if (wallet) {
          this.ngZone.run(() => {
            wallet.loaded = false;
            wallet.staking = is_mining;
            if (wallet_state === 2) { // ready
              wallet.loaded = true;
            }
            if (wallet_state === 3) { // error
              // safe.error = true;
            }
            wallet.balance = data.balance;
            wallet.unlocked_balance = data.unlocked_balance;
            wallet.mined_total = data.minied_total;
          });
        }
      });


      this.backend.eventSubscribe('wallet_sync_progress', (data) => {
        console.log('----------------- wallet_sync_progress -----------------');
        console.log(data);

        const wallet = this.variablesService.getWallet(data.wallet_id);
        if (wallet) {
          this.ngZone.run(() => {
            wallet.progress = data.progress;
            if (wallet.progress === 0) {
              wallet.loaded = false;
            } else if (wallet.progress === 100) {
              wallet.loaded = true;
            }
          });
        }
      });


      this.backend.eventSubscribe('update_daemon_state', (data) => {
        console.log('----------------- update_daemon_state -----------------');
        console.log('DAEMON:' + data.daemon_network_state);
        console.log(data);
        this.variablesService.exp_med_ts = data['expiration_median_timestamp'] + 600 + 1;
        // this.variablesService.height_app = data.height;
        this.variablesService.setHeightApp(data.height);

        this.ngZone.run(() => {
          this.variablesService.daemon_state = data.daemon_network_state;
          if (data.daemon_network_state === 1) {
            const max = data.max_net_seen_height - data.synchronization_start_height;
            const current = data.height - data.synchronization_start_height;
            const return_val = Math.floor((current * 100 / max) * 100) / 100;
            if (max === 0 || return_val < 0) {
              this.variablesService.sync.progress_value = 0;
              this.variablesService.sync.progress_value_text = '0.00';
            } else if (return_val >= 100) {
              this.variablesService.sync.progress_value = 100;
              this.variablesService.sync.progress_value_text = '99.99';
            } else {
              this.variablesService.sync.progress_value = return_val;
              this.variablesService.sync.progress_value_text = return_val.toFixed(2);
            }
          }
        });
      });

      this.backend.eventSubscribe('money_transfer', (data) => {
        console.log('----------------- money_transfer -----------------');
        console.log(data);

        if (!data.ti) {
          return;
        }

        const wallet_id = data.wallet_id;
        const tr_info = data.ti;

        const safe = this.variablesService.getWallet(wallet_id);

        if (safe) {
          this.ngZone.run(() => {

            if (!safe.loaded) {
              safe.balance = data.balance;
              safe.unlocked_balance = data.unlocked_balance;
            } else {
              safe.balance = data.balance;
              safe.unlocked_balance = data.unlocked_balance;
            }

            if (tr_info.tx_type === 6) {
              this.variablesService.setRefreshStacking(wallet_id);
            }

            let tr_exists = safe.excluded_history.some(elem => elem.tx_hash === tr_info.tx_hash);
            tr_exists = (!tr_exists) ? safe.history.some(elem => elem.tx_hash === tr_info.tx_hash) : tr_exists;

            safe.prepareHistory([tr_info]);

            if (tr_info.hasOwnProperty('contract')) {
              const exp_med_ts = this.variablesService.exp_med_ts;
              const height_app = this.variablesService.height_app;

              const contract = tr_info.contract[0];

              if (tr_exists) {
                for (let i = 0; i < safe.contracts.length; i++) {
                  if (safe.contracts[i].contract_id === contract.contract_id && safe.contracts[i].is_a === contract.is_a) {
                    safe.contracts[i].cancel_expiration_time = contract.cancel_expiration_time;
                    safe.contracts[i].expiration_time = contract.expiration_time;
                    safe.contracts[i].height = contract.height;
                    safe.contracts[i].timestamp = contract.timestamp;
                    break;
                  }
                }
                // $rootScope.getContractsRecount();
                return;
              }

              if (contract.state === 1 && contract.expiration_time < exp_med_ts) {
                contract.state = 110;
              } else if (contract.state === 5 && contract.cancel_expiration_time < exp_med_ts) {
                contract.state = 130;
              } else if (contract.state === 1) {
                const searchResult2 = this.variablesService.settings.notViewedContracts.find(elem => elem.state === 110 && elem.is_a === contract.is_a && elem.contract_id === contract.contract_id);
                if (searchResult2) {
                  if (searchResult2.time === contract.expiration_time) {
                    contract.state = 110;
                  } else {
                    for (let j = 0; j < this.variablesService.settings.notViewedContracts.length; j++) {
                      if (this.variablesService.settings.notViewedContracts[j].contract_id === contract.contract_id && this.variablesService.settings.notViewedContracts[j].is_a === contract.is_a) {
                        this.variablesService.settings.notViewedContracts.splice(j, 1);
                        break;
                      }
                    }
                    for (let j = 0; j < this.variablesService.settings.viewedContracts.length; j++) {
                      if (this.variablesService.settings.viewedContracts[j].contract_id === contract.contract_id && this.variablesService.settings.viewedContracts[j].is_a === contract.is_a) {
                        this.variablesService.settings.viewedContracts.splice(j, 1);
                        break;
                      }
                    }
                  }
                }
              } else if (contract.state === 2 && (contract.height === 0 || (height_app - contract.height) < 10)) {
                contract.state = 201;
              } else if (contract.state === 2) {
                const searchResult3 = this.variablesService.settings.viewedContracts.some(elem => elem.state === 120 && elem.is_a === contract.is_a && elem.contract_id === contract.contract_id);
                if (searchResult3) {
                  contract.state = 120;
                }
              } else if (contract.state === 5) {
                const searchResult4 = this.variablesService.settings.notViewedContracts.find(elem => elem.state === 130 && elem.is_a === contract.is_a && elem.contract_id === contract.contract_id);
                if (searchResult4) {
                  if (searchResult4.time === contract.cancel_expiration_time) {
                    contract.state = 130;
                  } else {
                    for (let j = 0; j < this.variablesService.settings.notViewedContracts.length; j++) {
                      if (this.variablesService.settings.notViewedContracts[j].contract_id === contract.contract_id && this.variablesService.settings.notViewedContracts[j].is_a === contract.is_a) {
                        this.variablesService.settings.notViewedContracts.splice(j, 1);
                        break;
                      }
                    }
                    for (let j = 0; j < this.variablesService.settings.viewedContracts.length; j++) {
                      if (this.variablesService.settings.viewedContracts[j].contract_id === contract.contract_id && this.variablesService.settings.viewedContracts[j].is_a === contract.is_a) {
                        this.variablesService.settings.viewedContracts.splice(j, 1);
                        break;
                      }
                    }
                  }
                }
              } else if (contract.state === 6 && (contract.height === 0 || (height_app - contract.height) < 10)) {
                contract.state = 601;
              }

              const searchResult = this.variablesService.settings.viewedContracts.some(elem => elem.state === contract.state && elem.is_a === contract.is_a && elem.contract_id === contract.contract_id);
              contract.is_new = !searchResult;

              contract['private_detailes'].a_pledge = contract['private_detailes'].a_pledge.plus(contract['private_detailes'].to_pay);

              let findContract = false;
              for (let i = 0; i < safe.contracts.length; i++) {
                if (safe.contracts[i].contract_id === contract.contract_id && safe.contracts[i].is_a === contract.is_a) {
                  for (const prop in contract) {
                    if (contract.hasOwnProperty(prop)) {
                      safe.contracts[i][prop] = contract[prop];
                    }
                  }
                  findContract = true;
                  break;
                }
              }
              if (findContract === false) {
                safe.contracts.push(contract);
              }
              safe.recountNewContracts();
            }

          });
        }
      });


      this.backend.eventSubscribe('money_transfer_cancel', (data) => {
        console.log('----------------- money_transfer_cancel -----------------');
        console.log(data);

        // if (!data.ti) {
        //   return;
        // }
        //
        // var wallet_id = data.wallet_id;
        // var tr_info = data.ti;
        // var safe = $rootScope.getSafeById(wallet_id);
        // if (safe) {
        //   if ( tr_info.hasOwnProperty("contract") ){
        //     for (var i = 0; i < $rootScope.contracts.length; i++) {
        //       if ($rootScope.contracts[i].contract_id === tr_info.contract[0].contract_id && $rootScope.contracts[i].is_a === tr_info.contract[0].is_a) {
        //         if ($rootScope.contracts[i].state === 1 || $rootScope.contracts[i].state === 110) {
        //           $rootScope.contracts[i].isNew = true;
        //           $rootScope.contracts[i].state = 140;
        //           $rootScope.getContractsRecount(); //escrow_code
        //         }
        //         break;
        //       }
        //     }
        //   }
        //   angular.forEach(safe.history, function (tr_item, key) {
        //     if (tr_item.tx_hash === tr_info.tx_hash) {
        //       safe.history.splice(key, 1);
        //     }
        //   });
        //
        //   var error_tr = '';
        //   switch (tr_info.tx_type) {
        //     case 0:
        //       error_tr = $filter('translate')('ERROR_GUI_TX_TYPE_NORMAL') + '<br>' +
        //         tr_info.tx_hash + '<br>' + safe.name + '<br>' + safe.address + '<br>' +
        //         $filter('translate')('ERROR_GUI_TX_TYPE_NORMAL_TO') + ' ' + $rootScope.moneyParse(tr_info.amount) + ' ' +
        //         $filter('translate')('ERROR_GUI_TX_TYPE_NORMAL_END');
        //       informer.error(error_tr);
        //       break;
        //     case 1:
        //       informer.error('ERROR_GUI_TX_TYPE_PUSH_OFFER');
        //       break;
        //     case 2:
        //       informer.error('ERROR_GUI_TX_TYPE_UPDATE_OFFER');
        //       break;
        //     case 3:
        //       informer.error('ERROR_GUI_TX_TYPE_CANCEL_OFFER');
        //       break;
        //     case 4:
        //       error_tr = $filter('translate')('ERROR_GUI_TX_TYPE_NEW_ALIAS') + '<br>' +
        //         tr_info.tx_hash + '<br>' + safe.name + '<br>' + safe.address + '<br>' +
        //         $filter('translate')('ERROR_GUI_TX_TYPE_NEW_ALIAS_END');
        //       informer.error(error_tr);
        //       break;
        //     case 5:
        //       error_tr = $filter('translate')('ERROR_GUI_TX_TYPE_UPDATE_ALIAS') + '<br>' +
        //         tr_info.tx_hash + '<br>' + safe.name + '<br>' + safe.address + '<br>' +
        //         $filter('translate')('ERROR_GUI_TX_TYPE_NEW_ALIAS_END');
        //       informer.error(error_tr);
        //       break;
        //     case 6:
        //       informer.error('ERROR_GUI_TX_TYPE_COIN_BASE');
        //       break;
        //   }
        // }
      });

      this.intervalUpdateContractsState = setInterval(() => {
        this.variablesService.wallets.forEach((wallet) => {
          wallet.contracts.forEach((contract) => {
            if (contract.state === 201 && contract.height !== 0 && (this.variablesService.height_app - contract.height) >= 10) {
              contract.state = 2;
              contract.is_new = true;
              console.warn('need check state in contracts');
            } else if (contract.state === 601 && contract.height !== 0 && (this.variablesService.height_app - contract.height) >= 10) {
              contract.state = 6;
              contract.is_new = true;
            }
          });
        });
      }, 30000);


      this.backend.getAppData((status, data) => {
        if (data && Object.keys(data).length > 0) {
          for (const key in data) {
            if (data.hasOwnProperty(key) && this.variablesService.settings.hasOwnProperty(key)) {
              this.variablesService.settings[key] = data[key];
            }
          }
          if (this.variablesService.settings.hasOwnProperty('theme') && ['dark', 'white', 'gray'].indexOf(this.variablesService.settings.theme) !== -1) {
            this.renderer.addClass(document.body, 'theme-' + this.variablesService.settings.theme);
          } else {
            this.renderer.addClass(document.body, 'theme-' + this.variablesService.defaultTheme);
          }
        } else {
          this.variablesService.settings.theme = this.variablesService.defaultTheme;
          this.renderer.addClass(document.body, 'theme-' + this.variablesService.settings.theme);
        }

        if (this.router.url !== '/login') {
          this.backend.haveSecureAppData((statusPass) => {
            if (statusPass) {
              this.ngZone.run(() => {
                this.router.navigate(['/login'], {queryParams: {type: 'auth'}});
              });
            } else {
              this.ngZone.run(() => {
                this.router.navigate(['/login'], {queryParams: {type: 'reg'}});
              });
            }
          });
        }
      });
    }, error => {
      console.log(error);
    });
    this.getMoneyEquivalent();
  }

  getMoneyEquivalent() {
    this.http.get('https://api.coinmarketcap.com/v2/ticker/406').subscribe(
      result => {
        if (result.hasOwnProperty('data')) {
          this.variablesService.moneyEquivalent = result['data']['quotes']['USD']['price'];
        }
      },
      error => {
        setTimeout(() => {
          this.getMoneyEquivalent();
        }, 60000);
        console.warn('Error coinmarketcap', error);
      }
    );
  }


  contextMenuCopy(target) {
    if (target && (target['nodeName'].toUpperCase() === 'TEXTAREA' || target['nodeName'].toUpperCase() === 'INPUT')) {
      const start = (target['contextSelectionStart']) ? 'contextSelectionStart' : 'selectionStart';
      const end = (target['contextSelectionEnd']) ? 'contextSelectionEnd' : 'selectionEnd';
      const canUseSelection = ((target[start]) || (target[start] === '0'));
      const SelectedText = (canUseSelection) ? target['value'].substring(target[start], target[end]) : target['value'];
      this.backend.setClipboard(String(SelectedText));
    }
  }

  contextMenuPaste(target) {
    if (target && (target['nodeName'].toUpperCase() === 'TEXTAREA' || target['nodeName'].toUpperCase() === 'INPUT')) {
      this.backend.getClipboard((status, clipboard) => {
        clipboard = String(clipboard);
        if (typeof clipboard !== 'string' || clipboard.length) {
          const start = (target['contextSelectionStart']) ? 'contextSelectionStart' : 'selectionStart';
          const end = (target['contextSelectionEnd']) ? 'contextSelectionEnd' : 'selectionEnd';
          const canUseSelection = ((target[start]) || (target[start] === '0'));
          let _pre = '';
          let _aft = '';
          if (canUseSelection) {
            _pre = target['value'].substring(0, target[start]);
            _aft = target['value'].substring(target[end], target['value'].length);
          }
          let text = (!canUseSelection) ? clipboard : _pre + clipboard + _aft;
          if (target['maxLength'] && parseInt(target['maxLength'], 10) > 0) {
            text = text.substr(0, parseInt(target['maxLength'], 10));
          }
          target['value'] = text;
          target['focus']();
        }
      });
    }
  }

  contextMenuSelect(target) {
    if (target && (target['nodeName'].toUpperCase() === 'TEXTAREA' || target['nodeName'].toUpperCase() === 'INPUT')) {
      target['focus']();
      setTimeout(() => {
        target['select']();
      });
    }
  }

  ngOnDestroy() {
    if (this.intervalUpdateContractsState) {
      clearInterval(this.intervalUpdateContractsState);
    }
  }

}
