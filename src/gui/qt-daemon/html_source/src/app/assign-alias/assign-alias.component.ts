import {Component, NgZone, OnInit, OnDestroy} from '@angular/core';
import {FormGroup, FormControl, Validators} from '@angular/forms';
import {Location} from '@angular/common';
import {Router} from '@angular/router';
import {BackendService} from '../_helpers/services/backend.service';
import {VariablesService} from '../_helpers/services/variables.service';
import {ModalService} from '../_helpers/services/modal.service';
import {Wallet} from '../_helpers/models/wallet.model';
import {MoneyToIntPipe} from '../_helpers/pipes/money-to-int.pipe';
import {IntToMoneyPipe} from '../_helpers/pipes/int-to-money.pipe';
import BigNumber from 'bignumber.js';
import {Subscription} from 'rxjs';

@Component({
  selector: 'app-assign-alias',
  templateUrl: './assign-alias.component.html',
  styleUrls: ['./assign-alias.component.scss']
})
export class AssignAliasComponent implements OnInit, OnDestroy {

  wallet: Wallet;
  assignForm = new FormGroup({
    name: new FormControl('', [Validators.required, Validators.pattern(/^@?[a-z0-9\.\-]{6,25}$/)]),
    comment: new FormControl('', [(g: FormControl) => {
      if (g.value > this.variablesService.maxCommentLength) {
        return {'maxLength': true};
      } else {
        return null;
      }
    }])
  });
  assignFormSubscription: Subscription;
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
    private router: Router,
    private backend: BackendService,
    public variablesService: VariablesService,
    private modalService: ModalService,
    private moneyToInt: MoneyToIntPipe,
    private intToMoney: IntToMoneyPipe
  ) {}

  ngOnInit() {
    this.wallet = this.variablesService.currentWallet;
    this.assignFormSubscription = this.assignForm.get('name').valueChanges.subscribe(value => {
      this.canRegister = false;
      this.alias.exists = false;
      const newName = value.toLowerCase().replace('@', '');
      if (!(this.assignForm.controls['name'].errors && this.assignForm.controls['name'].errors.hasOwnProperty('pattern')) && newName.length >= 6 && newName.length <= 25) {
        this.backend.getAliasByName(newName, status => {
          this.ngZone.run(() => {
            this.alias.exists = status;
          });
          if (!status) {
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
    const alias = this.backend.getWalletAlias(this.wallet.address);
    if (alias.hasOwnProperty('name')) {
      this.modalService.prepareModal('info', 'ASSIGN_ALIAS.ONE_ALIAS');
    } else {
      this.alias.comment = this.assignForm.get('comment').value;
      this.backend.registerAlias(this.wallet.wallet_id, this.alias.name, this.wallet.address, this.alias.fee, this.alias.comment, this.alias.rewardOriginal, (status, data) => {
        if (status) {
          this.wallet.wakeAlias = true;
          this.modalService.prepareModal('info', 'ASSIGN_ALIAS.REQUEST_ADD_REG');
          this.ngZone.run(() => {
            this.router.navigate(['/wallet/' + this.wallet.wallet_id]);
          });
        }
      });
    }
  }

  back() {
    this.location.back();
  }

  ngOnDestroy() {
    this.assignFormSubscription.unsubscribe();
  }
}
