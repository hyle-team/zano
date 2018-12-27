import {Component, OnInit, OnDestroy, NgZone} from '@angular/core';
import {ActivatedRoute} from '@angular/router';
import {FormControl, FormGroup, Validators} from '@angular/forms';
import {BackendService} from '../_helpers/services/backend.service';
import {VariablesService} from '../_helpers/services/variables.service';
import {Location} from '@angular/common';
import {IntToMoneyPipe} from '../_helpers/pipes/int-to-money.pipe';

@Component({
  selector: 'app-purchase',
  templateUrl: './purchase.component.html',
  styleUrls: ['./purchase.component.scss']
})
export class PurchaseComponent implements OnInit, OnDestroy {
  currentWalletId;
  newPurchase = false;
  parentRouting;
  historyBlock;

  purchaseForm = new FormGroup({
    description: new FormControl('', Validators.required),
    seller: new FormControl('', Validators.required),
    amount: new FormControl(null, Validators.required),
    yourDeposit: new FormControl(null, Validators.required),
    sellerDeposit: new FormControl(null, Validators.required),
    sameAmount: new FormControl(false),
    comment: new FormControl(''),
    fee: new FormControl('0.01'),
    payment: new FormControl('')
  });

  additionalOptions = false;
  currentContract = null;

  constructor(
    private route: ActivatedRoute,
    private backend: BackendService,
    private variablesService: VariablesService,
    private ngZone: NgZone,
    private location: Location,
    private intToMoneyPipe: IntToMoneyPipe
  ) {
  }

  checkAndChangeHistory() {
    if (this.currentContract.state === 201) {
      this.historyBlock = this.variablesService.currentWallet.history.find(item => item.tx_type === 8 && item.contract[0].contract_id === this.currentContract.contract_id && item.contract[0].is_a === this.currentContract.is_a);
    } else if (this.currentContract.state === 601) {
      this.historyBlock = this.variablesService.currentWallet.history.find(item => item.tx_type === 12 && item.contract[0].contract_id === this.currentContract.contract_id && item.contract[0].is_a === this.currentContract.is_a);
    }
  }

  ngOnInit() {
    this.parentRouting = this.route.parent.params.subscribe(params => {
      this.currentWalletId = params['id'];
    });
    this.route.params.subscribe(params => {
      if (params.hasOwnProperty('id')) {
        this.currentContract = this.variablesService.currentWallet.getContract(params['id']);
        this.purchaseForm.setValue({
          description: this.currentContract.private_detailes.t,
          seller: this.currentContract.private_detailes.b_addr,
          amount: this.intToMoneyPipe.transform(this.currentContract.private_detailes.to_pay),
          yourDeposit: this.intToMoneyPipe.transform(this.currentContract.private_detailes.a_pledge),
          sellerDeposit: this.intToMoneyPipe.transform(this.currentContract.private_detailes.b_pledge),
          sameAmount: false,
          comment: this.currentContract.private_detailes.c,
          fee: '0.01',
          payment: this.currentContract.payment_id
        });
        this.newPurchase = false;


        // todo original code watch height_v
        if (this.currentContract.state === 201 && this.currentContract.height !== 0 && (this.variablesService.height_app - this.currentContract.height) >= 10) {
          this.currentContract.state = 2;
          this.currentContract.is_new = true;
          this.variablesService.currentWallet.recountNewContracts();
        } else if (this.currentContract.state === 601 && this.currentContract.height !== 0 && (this.variablesService.height_app - this.currentContract.height) >= 10) {
          this.currentContract.state = 6;
          this.currentContract.is_new = true;
          this.variablesService.currentWallet.recountNewContracts();
        }


        if (this.currentContract.is_new) {
          if (this.currentContract.is_a && this.currentContract.state === 2) {
            this.currentContract.state = 120;
          }
          if (this.currentContract.state === 130 && this.currentContract.cancel_expiration_time !== 0 && this.currentContract.cancel_expiration_time < this.variablesService.exp_med_ts) {
            this.currentContract.state = 2;
          }

          this.variablesService.settings.viewedContracts = (this.variablesService.settings.viewedContracts) ? this.variablesService.settings.viewedContracts : [];
          let findViewedCont = false;
          for (let j = 0; j < this.variablesService.settings.viewedContracts.length; j++) {
            if (this.variablesService.settings.viewedContracts[j].contract_id === this.currentContract.contract_id && this.variablesService.settings.viewedContracts[j].is_a === this.currentContract.is_a) {
              this.variablesService.settings.viewedContracts[j].state = this.currentContract.state;
              findViewedCont = true;
              break;
            }
          }
          if (!findViewedCont) {
            this.variablesService.settings.viewedContracts.push({
              contract_id: this.currentContract.contract_id,
              is_a: this.currentContract.is_a,
              state: this.currentContract.state
            });
          }
          this.currentContract.is_new = false;

          // todo need remove timeout
          setTimeout(() => {
            this.variablesService.currentWallet.recountNewContracts();
          }, 0);

          this.checkAndChangeHistory();

        }

      } else {
        this.newPurchase = true;
      }
    });
  }

  toggleOptions() {
    this.additionalOptions = !this.additionalOptions;
  }

  getProgressBarWidth() {
    if (this.newPurchase) {
      return '9rem';
    } else {
      return '50%';
    }
  }

  sameAmountChange() {
    if (this.purchaseForm.get('sameAmount').value) {
      this.purchaseForm.get('sellerDeposit').clearValidators();
      this.purchaseForm.get('sellerDeposit').updateValueAndValidity();
    } else {
      this.purchaseForm.get('sellerDeposit').setValidators([Validators.required]);
      this.purchaseForm.get('sellerDeposit').updateValueAndValidity();
    }
  }

  checkAddressValidation() {
    if (this.purchaseForm.get('seller').value) {
      this.backend.validateAddress(this.purchaseForm.get('seller').value, (valid_status) => {
        if (valid_status === false) {
          this.ngZone.run(() => {
            this.purchaseForm.get('seller').setErrors({address_not_valid: true});
          });
        }
      });
    }
  }

  createPurchase() {
    if (this.purchaseForm.valid) {
      if (this.purchaseForm.get('sameAmount').value) {
        this.purchaseForm.get('sellerDeposit').setValue(this.purchaseForm.get('amount').value);
      }
      this.backend.createProposal(
        this.variablesService.currentWallet.wallet_id,
        this.purchaseForm.get('description').value,
        this.purchaseForm.get('comment').value,
        this.variablesService.currentWallet.address,
        this.purchaseForm.get('seller').value,
        this.purchaseForm.get('amount').value,
        this.purchaseForm.get('yourDeposit').value,
        this.purchaseForm.get('sellerDeposit').value,
        12,
        this.purchaseForm.get('payment').value,
        () => {
          this.back();
        });
    }
  }

  back() {
    this.location.back();
  }

  acceptState() {
    this.backend.acceptProposal(this.currentWalletId, this.currentContract.contract_id, (accept_status) => {
      if (accept_status) {
        alert('You have accepted the contract proposal. Please wait for the pledges to be made');
        this.back();
      }
    });
  }

  ignoredContract() {
    this.variablesService.settings.notViewedContracts = (this.variablesService.settings.notViewedContracts) ? this.variablesService.settings.notViewedContracts : [];
    let findViewedCont = false;
    for (let j = 0; j < this.variablesService.settings.notViewedContracts.length; j++) {
      if (this.variablesService.settings.notViewedContracts[j].contract_id === this.currentContract.contract_id && this.variablesService.settings.notViewedContracts[j].is_a === this.currentContract.is_a) {
        this.variablesService.settings.notViewedContracts[j].state = 110;
        this.variablesService.settings.notViewedContracts[j].time = this.currentContract.expiration_time;
        findViewedCont = true;
        break;
      }
    }
    if (!findViewedCont) {
      this.variablesService.settings.notViewedContracts.push({
        contract_id: this.currentContract.contract_id,
        is_a: this.currentContract.is_a,
        state: 110,
        time: this.currentContract.expiration_time
      });
    }
    this.currentContract.is_new = true;
    this.currentContract.state = 110;
    this.currentContract.time = this.currentContract.expiration_time;

    this.variablesService.currentWallet.recountNewContracts();
    alert('You have ignored the contract proposal');
    this.back();
  }


  productNotGot() {
    this.backend.releaseProposal(this.currentWalletId, this.currentContract.contract_id, 'REL_B', (release_status) => {
      if (release_status) {
        alert('The pledges have been nullified.');
        this.back();
      }
    });
  }

  dealsDetailsFinish() {
    this.backend.releaseProposal(this.currentWalletId, this.currentContract.contract_id, 'REL_N', (release_status) => {
      if (release_status) {
        alert('The contract is complete. The payment has been sent.');
        this.back();
      }
    });
  }

  dealsDetailsCancel() {
    this.backend.requestCancelContract(this.currentWalletId, this.currentContract.contract_id, 12, (cancel_status) => {
      if (cancel_status) {
        alert('Proposal to cancel contract sent to seller');
        this.back();
      }
    });
  }

  dealsDetailsDontCanceling() {
    this.variablesService.settings.notViewedContracts = this.variablesService.settings.notViewedContracts ? this.variablesService.settings.notViewedContracts : [];
    let findViewedCont = false;
    for (let j = 0; j < this.variablesService.settings.notViewedContracts.length; j++) {
      if (this.variablesService.settings.notViewedContracts[j].contract_id === this.currentContract.contract_id && this.variablesService.settings.notViewedContracts[j].is_a === this.currentContract.is_a) {
        this.variablesService.settings.notViewedContracts[j].state = 130;
        this.variablesService.settings.notViewedContracts[j].time = this.currentContract.cancel_expiration_time;
        findViewedCont = true;
        break;
      }
    }
    if (!findViewedCont) {
      this.variablesService.settings.notViewedContracts.push({
        contract_id: this.currentContract.contract_id,
        is_a: this.currentContract.is_a,
        state: 130,
        time: this.currentContract.cancel_expiration_time
      });
    }
    this.currentContract.is_new = true;
    this.currentContract.state = 130;
    this.currentContract.time = this.currentContract.cancel_expiration_time;

    this.variablesService.currentWallet.recountNewContracts();
    alert('You have ignored the proposal to cancel the contract');
    this.back();
  }

  dealsDetailsSellerCancel() {
    this.backend.acceptCancelContract(this.currentWalletId, this.currentContract.contract_id, (accept_status) => {
      if (accept_status) {
        alert('The contract is being cancelled. Please wait for the pledge to be returned');
        this.back();
      }
    });
  }

  ngOnDestroy() {
    this.parentRouting.unsubscribe();
  }

}
