import {Component, NgZone, OnInit} from '@angular/core';
import {Location} from "@angular/common";
import {Router} from "@angular/router";
import {BackendService} from "../_helpers/services/backend.service";
import {VariablesService} from "../_helpers/services/variables.service";
import {ModalService} from "../_helpers/services/modal.service";
import {Wallet} from "../_helpers/models/wallet.model";

@Component({
  selector: 'app-transfer-alias',
  templateUrl: './transfer-alias.component.html',
  styleUrls: ['./transfer-alias.component.scss']
})
export class TransferAliasComponent implements OnInit {

  wallet: Wallet;
  alias: any;
  transferAddress = '';
  transferAddressValid: boolean;
  transferAddressAlias: boolean;
  permissionSend: boolean;
  notEnoughMoney: boolean;
  processingRequst = false;

  constructor(
    private location: Location,
    private router: Router,
    private backend: BackendService,
    private variablesService: VariablesService,
    private modalService: ModalService,
    private ngZone: NgZone
  ) {}

  ngOnInit() {
    this.wallet = this.variablesService.currentWallet;
    const alias = this.backend.getWalletAlias(this.wallet.address);
    this.alias = {
      name: alias.name,
      address: alias.address,
      comment: alias.comment,
      tracking_key: alias.tracking_key
    };
    this.notEnoughMoney = this.wallet.unlocked_balance.isLessThan(this.variablesService.default_fee_big);
  }

  changeAddress() {
    this.backend.validateAddress(this.transferAddress, status => {
      this.transferAddressValid = status;
      if (status) {
        this.backend.getPoolInfo((statusPool, dataPool) => {
          if (dataPool.hasOwnProperty('aliases_que') && dataPool.aliases_que.length) {
            this.setStatus(!~dataPool.aliases_que.searchBy('address', this.transferAddress));
          } else {
            this.setStatus(status);
          }
        });
      } else {
        this.setStatus(false);
      }
    });
  }

  setStatus(statusSet) {
    this.permissionSend = statusSet;
    if (statusSet) {
      this.backend.getAliasByAddress(this.transferAddress, (status, data) => {
        this.ngZone.run(() => {
          if (status) {
            this.transferAddressAlias = true;
            this.permissionSend = false;
          } else {
            this.transferAddressAlias = false;
          }
        });
      });
    } else {
      this.ngZone.run(() => {
        this.transferAddressAlias = false;
      });
    }
  }

  transferAlias() {
    if (this.processingRequst || !this.permissionSend || !this.transferAddressValid || this.notEnoughMoney) {
      return;
    }
    this.processingRequst = true;
    const newAlias = {
      name: this.alias.name,
      address: this.transferAddress,
      comment: this.alias.comment,
      tracking_key: this.alias.tracking_key
    };
    this.backend.updateAlias(this.wallet.wallet_id, newAlias, this.variablesService.default_fee, (status, data) => {
      if (status && data.hasOwnProperty('success') && data.success) {
        this.modalService.prepareModal('info', 'TRANSFER_ALIAS.REQUEST_SEND_REG');
        this.ngZone.run(() => {
          this.router.navigate(['/wallet/' + this.wallet.wallet_id]);
        });
      }
      this.processingRequst = false;
    });
  }

  back() {
    this.location.back();
  }
}
