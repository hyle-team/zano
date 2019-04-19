import {Component, NgZone, OnInit} from '@angular/core';
import {FormGroup, FormControl, Validators} from '@angular/forms';
import {Router} from '@angular/router';
import {BackendService} from '../_helpers/services/backend.service';
import {VariablesService} from '../_helpers/services/variables.service';
import {ModalService} from '../_helpers/services/modal.service';
import {Wallet} from '../_helpers/models/wallet.model';
import {TranslateService} from '@ngx-translate/core';

@Component({
  selector: 'app-restore-wallet',
  templateUrl: './restore-wallet.component.html',
  styleUrls: ['./restore-wallet.component.scss']
})
export class RestoreWalletComponent implements OnInit {

  restoreForm = new FormGroup({
    name: new FormControl('', [Validators.required, (g: FormControl) => {
      for (let i = 0; i < this.variablesService.wallets.length; i++) {
        if (g.value === this.variablesService.wallets[i].name) {
          return {'duplicate': true};
        }
      }
      return null;
    }]),
    key: new FormControl('', Validators.required),
    password: new FormControl(''),
    confirm: new FormControl('')
  }, function (g: FormGroup) {
    return g.get('password').value === g.get('confirm').value ? null : {'confirm_mismatch': true};
  });

  wallet = {
    id: ''
  };

  walletSaved = false;
  walletSavedName = '';
  progressWidth = '9rem';

  constructor(
    private router: Router,
    private backend: BackendService,
    private variablesService: VariablesService,
    private modalService: ModalService,
    private ngZone: NgZone,
    private translate: TranslateService
  ) {
  }

  ngOnInit() {
  }


  createWallet() {
    this.ngZone.run(() => {
      this.progressWidth = '100%';
      this.router.navigate(['/seed-phrase'], {queryParams: {wallet_id: this.wallet.id}});
    });
  }

  saveWallet() {
    if (this.restoreForm.valid && this.restoreForm.get('name').value.length <= this.variablesService.maxWalletNameLength) {
      this.backend.isValidRestoreWalletText(this.restoreForm.get('key').value, (valid_status, valid_data) => {
        if (valid_data !== 'TRUE') {
          this.ngZone.run(() => {
            this.restoreForm.get('key').setErrors({key_not_valid: true});
          });
        } else {
          this.backend.saveFileDialog(this.translate.instant('RESTORE_WALLET.CHOOSE_PATH'), '*', this.variablesService.settings.default_path, (save_status, save_data) => {
            if (save_status) {
              this.variablesService.settings.default_path = save_data.path.substr(0, save_data.path.lastIndexOf('/'));
              this.walletSavedName = save_data.path.substr(save_data.path.lastIndexOf('/') + 1, save_data.path.length - 1);
              this.backend.restoreWallet(save_data.path, this.restoreForm.get('password').value, this.restoreForm.get('key').value, (restore_status, restore_data) => {
                if (restore_status) {
                  this.wallet.id = restore_data.wallet_id;
                  this.variablesService.opening_wallet = new Wallet(
                    restore_data.wallet_id,
                    this.restoreForm.get('name').value,
                    this.restoreForm.get('password').value,
                    restore_data['wi'].path,
                    restore_data['wi'].address,
                    restore_data['wi'].balance,
                    restore_data['wi'].unlocked_balance,
                    restore_data['wi'].mined_total,
                    restore_data['wi'].tracking_hey
                  );
                  this.variablesService.opening_wallet.alias = this.backend.getWalletAlias(this.variablesService.opening_wallet.address);
                  if (restore_data.recent_history && restore_data.recent_history.history) {
                    this.variablesService.opening_wallet.prepareHistory(restore_data.recent_history.history);
                  }
                  this.backend.getContracts(this.variablesService.opening_wallet.wallet_id, (contracts_status, contracts_data) => {
                    if (contracts_status && contracts_data.hasOwnProperty('contracts')) {
                      this.ngZone.run(() => {
                        this.variablesService.opening_wallet.prepareContractsAfterOpen(contracts_data.contracts, this.variablesService.exp_med_ts, this.variablesService.height_app, this.variablesService.settings.viewedContracts, this.variablesService.settings.notViewedContracts);
                      });
                    }
                  });
                  this.ngZone.run(() => {
                    this.walletSaved = true;
                    this.progressWidth = '50%';
                  });
                } else {
                  this.modalService.prepareModal('error', 'RESTORE_WALLET.NOT_CORRECT_FILE_OR_PASSWORD');
                }
              });
            }
          });
        }
      });
    }
  }


}
