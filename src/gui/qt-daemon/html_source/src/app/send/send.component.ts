import {Component, OnInit, OnDestroy, NgZone} from '@angular/core';
import {FormGroup, FormControl, Validators} from '@angular/forms';
import {ActivatedRoute} from '@angular/router';
import {BackendService} from '../_helpers/services/backend.service';
import {VariablesService} from '../_helpers/services/variables.service';
import {ModalService} from '../_helpers/services/modal.service';
import {BigNumber} from 'bignumber.js';

@Component({
  selector: 'app-send',
  templateUrl: './send.component.html',
  styleUrls: ['./send.component.scss']
})
export class SendComponent implements OnInit, OnDestroy {

  currentWalletId = null;
  parentRouting;
  sendForm = new FormGroup({
    address: new FormControl('', Validators.required),
    amount: new FormControl(null, [Validators.required, (g: FormControl) => {
      if (g.value === '0') {
        return {'zero': true};
      }
      return null;
    }]),
    comment: new FormControl(''),
    mixin: new FormControl(0, Validators.required),
    fee: new FormControl(this.variablesService.default_fee, [Validators.required, (g: FormControl) => {
      if ((new BigNumber(g.value)).isLessThan(this.variablesService.default_fee)) {
        return {'less_min': true};
      }
      return null;
    }])
  });
  additionalOptions = false;

  constructor(
    private route: ActivatedRoute,
    private backend: BackendService,
    private variablesService: VariablesService,
    private modalService: ModalService,
    private ngZone: NgZone
  ) {}

  ngOnInit() {
    this.parentRouting = this.route.parent.params.subscribe(params => {
      this.currentWalletId = params['id'];
      this.sendForm.reset({address: '', amount: null, comment: '', mixin: 0, fee: this.variablesService.default_fee});
    });
  }

  checkAddressValidation() {
    if (this.sendForm.get('address').value) {
      this.backend.validateAddress(this.sendForm.get('address').value, (valid_status) => {
        if (valid_status === false) {
          this.ngZone.run(() => {
            this.sendForm.get('address').setErrors({address_not_valid: true});
          });
        }
      });
    }
  }

  onSend() {
    if (this.sendForm.valid) {
      this.backend.validateAddress(this.sendForm.get('address').value, (valid_status) => {
        if (valid_status === false) {
          this.ngZone.run(() => {
            this.sendForm.get('address').setErrors({address_not_valid: true});
          });
        } else {
          this.backend.sendMoney(
            this.currentWalletId,
            this.sendForm.get('address').value,
            this.sendForm.get('amount').value,
            this.sendForm.get('fee').value,
            this.sendForm.get('mixin').value,
            this.sendForm.get('comment').value,
            (send_status, send_data) => {
              if (send_status) {
                this.modalService.prepareModal('success', 'SEND.SUCCESS_SENT');
                this.sendForm.reset({address: '', amount: null, comment: '', mixin: 0, fee: this.variablesService.default_fee});
              }
            });
        }
      });
    }
  }

  toggleOptions() {
    this.additionalOptions = !this.additionalOptions;
  }

  ngOnDestroy() {
    this.parentRouting.unsubscribe();
  }

}
