import {Component, OnInit, NgZone} from '@angular/core';
import {FormGroup, FormControl, Validators} from '@angular/forms';
import {Location} from "@angular/common";
import {BackendService} from '../_helpers/services/backend.service';
import {VariablesService} from '../_helpers/services/variables.service';
import {Wallet} from "../_helpers/models/wallet.model";
import {MoneyToIntPipe} from "../_helpers/pipes/money-to-int.pipe";
import {IntToMoneyPipe} from "../_helpers/pipes/int-to-money.pipe";
import BigNumber from "bignumber.js";

@Component({
  selector: 'app-assign-alias',
  templateUrl: './assign-alias.component.html',
  styleUrls: ['./assign-alias.component.scss']
})
export class AssignAliasComponent implements OnInit {

  wallet: Wallet;
  assignForm = new FormGroup({
    name: new FormControl('', [Validators.required, Validators.pattern(/^@?[a-z0-9\.\-]{6,25}$/)]),
    comment: new FormControl('')
  });
  alias = {
    name: '',
    fee: this.variablesService.default_fee,
    price: new BigNumber(0),
    reward: '0',
    rewardOriginal: '0',
    comment: '',
    exists: false
  };
  canRegister = false;
  notEnoughMoney = false;

  constructor(
    private ngZone: NgZone,
    private location: Location,
    private backend: BackendService,
    private variablesService: VariablesService,
    private moneyToInt: MoneyToIntPipe,
    private intToMoney: IntToMoneyPipe
  ) {
  }

  ngOnInit() {
    this.wallet = this.variablesService.currentWallet;
    this.assignForm.get('name').valueChanges.subscribe(value => {
      this.canRegister = false;
      let newName = value.toLowerCase().replace('@', '');
      if (!(this.assignForm.controls['name'].errors && this.assignForm.controls['name'].errors.hasOwnProperty('pattern')) && newName.length >= 6 && newName.length <= 25) {
        this.backend.getAliasByName(newName, status => {
          this.alias.exists = status;
          if (!this.alias.exists) {
            this.alias.price = new BigNumber(0);
            this.backend.getAliasCoast(newName, (statusPrice, dataPrice) => {
              this.ngZone.run(() => {
                if (statusPrice) {
                  this.alias.price = BigNumber.sum(dataPrice['coast'], this.variablesService.default_fee_big);
                }
                this.notEnoughMoney = this.alias.price.isGreaterThan(this.wallet.unlocked_balance);
                this.alias.reward = this.intToMoney.transform(this.alias.price, false);
                this.alias.rewardOriginal = this.intToMoney.transform(dataPrice['coast'], false);
                this.canRegister = !this.notEnoughMoney;
              });
            });
          } else {
            this.notEnoughMoney = false;
            this.alias.reward = '0';
            this.alias.rewardOriginal = '0';
          }
        });
      } else {
        this.notEnoughMoney = false;
        this.alias.reward = '0';
        this.alias.rewardOriginal = '0';
      }
      this.alias.name = newName;
    });
  }

  assignAlias() {
    /*let alias = getWalletAlias(wallet.address);
    if (alias.hasOwnProperty('name')) {
      informer.warning('INFORMER.ONE_ALIAS');
    } else {
      backend.registerAlias(wallet.wallet_id, this.alias.name, wallet.address, this.alias.fee, this.alias.comment, this.alias.rewardOriginal, function (status, data) {
        if (status) {
          service.unconfirmed_aliases.push({tx_hash: data.tx_hash, name: this.alias.name});
          wallet.wakeAlias = true;
          informer.success('INFORMER.REQUEST_ADD_REG');
        }
      });
    }*/
  }

  back() {
    this.location.back();
  }
}
