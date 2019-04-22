import {Component, OnInit, Input, NgZone} from '@angular/core';
import {VariablesService} from '../_helpers/services/variables.service';
import {Wallet} from '../_helpers/models/wallet.model';
import {BackendService} from '../_helpers/services/backend.service';
import {TranslateService} from '@ngx-translate/core';
import {ModalService} from '../_helpers/services/modal.service';

@Component({
  selector: 'app-open-wallet-modal',
  templateUrl: './open-wallet-modal.component.html',
  styleUrls: ['./open-wallet-modal.component.scss']
})
export class OpenWalletModalComponent implements OnInit {

  @Input() wallets;

  wallet = {
    name: '',
    path: '',
    pass: '',
    notFound: false,
    emptyPass: false
  };

  constructor(
    public variablesService: VariablesService,
    private backend: BackendService,
    private translate: TranslateService,
    private modalService: ModalService,
    private ngZone: NgZone,
  ) {
  }

  ngOnInit() {
    if (this.wallets.length) {
      this.wallet = this.wallets[0];
      this.wallet.pass = '';

      this.backend.openWallet(this.wallet.path, '', true, (status, data, error) => {
        if (error === 'FILE_NOT_FOUND') {
          this.wallet.notFound = true;
        }
        if (status) {
          this.wallet.pass = '';
          this.wallet.emptyPass = true;
          this.backend.closeWallet(data.wallet_id);
          this.openWallet();
        }
      });
    }
  }

  openWallet() {
    if (this.wallets.length === 0) {
      return;
    }
    this.backend.openWallet(this.wallet.path, this.wallet.pass, false, (open_status, open_data, open_error) => {
      if (open_error && open_error === 'FILE_NOT_FOUND') {
        let error_translate = this.translate.instant('OPEN_WALLET.FILE_NOT_FOUND1');
        error_translate += ':<br>' + this.wallet.path;
        error_translate += this.translate.instant('OPEN_WALLET.FILE_NOT_FOUND2');
        this.modalService.prepareModal('error', error_translate);
      } else {
        if (open_status || open_error === 'FILE_RESTORED') {

          let exists = false;
          this.variablesService.wallets.forEach((wallet) => {
            if (wallet.address === open_data['wi'].address) {
              exists = true;
            }
          });

          if (exists) {
            this.modalService.prepareModal('error', 'OPEN_WALLET.WITH_ADDRESS_ALREADY_OPEN');
            this.backend.closeWallet(open_data.wallet_id);
          } else {
            const new_wallet = new Wallet(
              open_data.wallet_id,
              this.wallet.name,
              this.wallet.pass,
              open_data['wi'].path,
              open_data['wi'].address,
              open_data['wi'].balance,
              open_data['wi'].unlocked_balance,
              open_data['wi'].mined_total,
              open_data['wi'].tracking_hey
            );
            new_wallet.alias = this.backend.getWalletAlias(new_wallet.address);
            if (open_data.recent_history && open_data.recent_history.history) {
              new_wallet.prepareHistory(open_data.recent_history.history);
            }
            this.backend.getContracts(open_data.wallet_id, (contracts_status, contracts_data) => {
              if (contracts_status && contracts_data.hasOwnProperty('contracts')) {
                this.ngZone.run(() => {
                  new_wallet.prepareContractsAfterOpen(
                    contracts_data.contracts,
                    this.variablesService.exp_med_ts,
                    this.variablesService.height_app,
                    this.variablesService.settings.viewedContracts,
                    this.variablesService.settings.notViewedContracts
                  );
                });
              }
            });
            this.variablesService.wallets.push(new_wallet);
            this.backend.runWallet(open_data.wallet_id);
            this.skipWallet();
          }
        }
      }
    });
  }

  skipWallet() {
    if (this.wallets.length) {
      this.wallets.splice(0, 1);
      this.ngOnInit();
    }
  }

}
