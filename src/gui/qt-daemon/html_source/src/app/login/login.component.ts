import {Component, NgZone, OnInit, OnDestroy} from '@angular/core';
import {FormGroup, FormControl} from '@angular/forms';
import {ActivatedRoute, Router} from '@angular/router';
import {BackendService} from '../_helpers/services/backend.service';
import {VariablesService} from '../_helpers/services/variables.service';
import {ModalService} from '../_helpers/services/modal.service';
import {Wallet} from '../_helpers/models/wallet.model';

@Component({
  selector: 'app-login',
  templateUrl: './login.component.html',
  styleUrls: ['./login.component.scss']
})
export class LoginComponent implements OnInit, OnDestroy {

  queryRouting;

  regForm = new FormGroup({
    password: new FormControl(''),
    confirmation: new FormControl('')
  }, function (g: FormGroup) {
    return g.get('password').value === g.get('confirmation').value ? null : {'mismatch': true};
  });

  authForm = new FormGroup({
    password: new FormControl('')
  });

  type = 'reg';
  currentPass = ''; 

  constructor(
    private route: ActivatedRoute,
    private router: Router,
    private backend: BackendService,
    public variablesService: VariablesService,
    private modalService: ModalService,
    private ngZone: NgZone
  ) {}

  ngOnInit() {
    this.queryRouting = this.route.queryParams.subscribe(params => {
      if (params.type) {
        this.type = params.type;
      }
    });
  }

  onSubmitCreatePass(): void {
    if (this.regForm.valid) {
      // this.variablesService.appPass = this.regForm.get('password').value; ||changeD it on 23.07 12:09||
      this.currentPass = this.regForm.get('password').value  //the pass what was written in input of login form by user

      this.backend.setMasterPassword({pass: this.currentPass}, (status, data) => {
        if (status) {
          this.variablesService.appLogin = true;
          this.variablesService.startCountdown();
          this.ngZone.run(() => {
            this.router.navigate(['/']);
          });
        } else {
          console.log(data['error_code']);
        }
      })
      /*
      this.backend.storeSecureAppData((status, data) => {
        if (status) {
          this.variablesService.appLogin = true;
          this.variablesService.startCountdown();
          this.ngZone.run(() => {
            this.router.navigate(['/']);
          });
        } else {
          console.log(data['error_code']);
        }
      });
      */
    }
  }

  onSkipCreatePass(): void {
    this.variablesService.appPass = '';
    this.ngZone.run(() => {
      this.variablesService.appLogin = true;
      this.router.navigate(['/']);
    });
  }

  onSubmitAuthPass(): void {
    
    if (this.authForm.valid) {
      const appPass = this.authForm.get('password').value;
      this.backend.getSecureAppData({pass: appPass}, (status, data) => {
        if (!data.error_code) {
          this.variablesService.appLogin = true;
          this.variablesService.startCountdown();
          this.variablesService.appPass = appPass;
          if (this.variablesService.wallets.length) {
            this.ngZone.run(() => {
              this.router.navigate(['/wallet/' + this.variablesService.wallets[0].wallet_id]);
            });
            return;
          }
          if (Object.keys(data).length !== 0) {
            let openWallets = 0;
            let runWallets = 0;
            data.forEach((wallet, wallet_index) => {
              this.backend.openWallet(wallet.path, wallet.pass, true, (open_status, open_data, open_error) => {
                if (open_status || open_error === 'FILE_RESTORED') {
                  openWallets++;
                  this.ngZone.run(() => {
                    const new_wallet = new Wallet(
                      open_data.wallet_id,
                      wallet.name,
                      wallet.pass,
                      open_data['wi'].path,
                      open_data['wi'].address,
                      open_data['wi'].balance,
                      open_data['wi'].unlocked_balance,
                      open_data['wi'].mined_total,
                      open_data['wi'].tracking_hey
                    );
                    new_wallet.alias = this.backend.getWalletAlias(new_wallet.address);
                    if (wallet.staking) {
                      new_wallet.staking = true;
                      this.backend.startPosMining(new_wallet.wallet_id);
                    } else {
                      new_wallet.staking = false;
                    }
                    if (open_data.recent_history && open_data.recent_history.history) {
                      new_wallet.prepareHistory(open_data.recent_history.history);
                    }
                    this.backend.getContracts(open_data.wallet_id, (contracts_status, contracts_data) => {
                      if (contracts_status && contracts_data.hasOwnProperty('contracts')) {
                        this.ngZone.run(() => {
                          new_wallet.prepareContractsAfterOpen(contracts_data.contracts, this.variablesService.exp_med_ts, this.variablesService.height_app, this.variablesService.settings.viewedContracts, this.variablesService.settings.notViewedContracts);
                        });
                      }
                    });
                    this.variablesService.wallets.push(new_wallet);
                    if (this.variablesService.wallets.length === 1) {
                      this.router.navigate(['/wallet/' + this.variablesService.wallets[0].wallet_id]);
                    }
                  });
                  this.backend.runWallet(open_data.wallet_id, (run_status) => {
                    if (run_status) {
                      runWallets++;
                    } else {
                      if (wallet_index === data.length - 1 && runWallets === 0) {
                        this.ngZone.run(() => {
                          this.router.navigate(['/']);
                        });
                      }
                    }
                  });
                } else {
                  if (wallet_index === data.length - 1 && openWallets === 0) {
                    this.ngZone.run(() => {
                      this.router.navigate(['/']);
                    });
                  }
                }
              });
            });
          } else {
            this.ngZone.run(() => {
              this.router.navigate(['/']);
            });
          }
        }
      });
    }
  }


  ngOnDestroy() {
    this.queryRouting.unsubscribe();
  }

}
