import {Component, OnInit, OnDestroy, NgZone, HostListener, Input} from '@angular/core';
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

  isOpen = false;
  localAliases = [];
  isModalDialogVisible = false;

  currentWalletId = null;
  parentRouting;
  sendForm = new FormGroup({
    address: new FormControl('', [Validators.required, (g: FormControl) => {
      this.localAliases = [];
      if (g.value) {
        if (g.value.indexOf('@') !== 0) {
          this.isOpen = false;
          this.backend.validateAddress(g.value, (valid_status) => {
            this.ngZone.run(() => {
              if (valid_status === false) {
                g.setErrors(Object.assign({'address_not_valid': true}, g.errors));
              } else {
                if (g.hasError('address_not_valid')) {
                  delete g.errors['address_not_valid'];
                  if (Object.keys(g.errors).length === 0) {
                    g.setErrors(null);
                  }
                }
              }
            });
          });
          return (g.hasError('address_not_valid')) ? {'address_not_valid': true} : null;
        } else {
          this.isOpen = true;
          this.localAliases = this.variablesService.aliases.filter((item) => {
            return item.name.indexOf(g.value) > -1;
          });
          if (!(/^@?[a-z0-9\.\-]{6,25}$/.test(g.value))) {
            g.setErrors(Object.assign({'alias_not_valid': true}, g.errors));
          } else {
            this.backend.getAliasByName(g.value.replace('@', ''), (alias_status) => {
              this.ngZone.run(() => {
                if (alias_status) {
                  if (g.hasError('alias_not_valid')) {
                    delete g.errors['alias_not_valid'];
                    if (Object.keys(g.errors).length === 0) {
                      g.setErrors(null);
                    }
                  }
                } else {
                  g.setErrors(Object.assign({'alias_not_valid': true}, g.errors));
                }
              });
            });
          }
          return (g.hasError('alias_not_valid')) ? {'alias_not_valid': true} : null;
        }
      }
      return null;
    }]),
    amount: new FormControl(null, [Validators.required, (g: FormControl) => {
      if (new BigNumber(g.value).eq(0)) {
        return {'zero': true};
      }
      return null;
    }]),
    comment: new FormControl(''),
    mixin: new FormControl(10, Validators.required),
    fee: new FormControl(this.variablesService.default_fee, [Validators.required, (g: FormControl) => {
      if ((new BigNumber(g.value)).isLessThan(this.variablesService.default_fee)) {
        return {'less_min': true};
      }
      return null;
    }]),
    hide: new FormControl(false)
  });
  additionalOptions = false;

  constructor(
    private route: ActivatedRoute,
    private backend: BackendService,
    public variablesService: VariablesService,
    private modalService: ModalService,
    private ngZone: NgZone
  ) {
  }

  addressMouseDown(e) {
    if (e['button'] === 0 && this.sendForm.get('address').value && this.sendForm.get('address').value.indexOf('@') === 0) {
      this.isOpen = true;
    }
  }

  setAlias(alias) {
    this.sendForm.get('address').setValue(alias);
  }

  @HostListener('document:click', ['$event.target'])
  public onClick(targetElement) {
    if (targetElement.id !== 'send-address' && this.isOpen) {
      this.isOpen = false;
    }
  }


  ngOnInit() {
    this.parentRouting = this.route.parent.params.subscribe(params => {
      this.currentWalletId = params['id'];
      this.sendForm.reset({
        address: this.variablesService.currentWallet.send_data['address'],
        amount: this.variablesService.currentWallet.send_data['amount'],
        comment: this.variablesService.currentWallet.send_data['comment'],
        mixin: this.variablesService.currentWallet.send_data['mixin'] || 10,
        fee: this.variablesService.currentWallet.send_data['fee'] || this.variablesService.default_fee,
        hide: this.variablesService.currentWallet.send_data['hide'] || false
      });
    });
  }

  showDialog() {
    this.isModalDialogVisible = true;
  }

  confirmed(confirmed: boolean) {
    if (confirmed) {
      this.onSend();
    }
    this.isModalDialogVisible = false;
  }

  onSend() {
    if (this.sendForm.valid) {
      if (this.sendForm.get('address').value.indexOf('@') !== 0) {
        this.backend.validateAddress(this.sendForm.get('address').value, (valid_status) => {
          if (valid_status === false) {
            this.ngZone.run(() => {
              this.sendForm.get('address').setErrors({'address_not_valid': true});
            });
          } else {
            this.backend.sendMoney(
              this.currentWalletId,
              this.sendForm.get('address').value,
              this.sendForm.get('amount').value,
              this.sendForm.get('fee').value,
              this.sendForm.get('mixin').value,
              this.sendForm.get('comment').value,
              this.sendForm.get('hide').value,
              (send_status) => {
                if (send_status) {
                  this.modalService.prepareModal('success', 'SEND.SUCCESS_SENT');
                  this.variablesService.currentWallet.send_data = {address: null, amount: null, comment: null, mixin: null, fee: null, hide: null};
                  this.sendForm.reset({address: null, amount: null, comment: null, mixin: 10, fee: this.variablesService.default_fee, hide: false});
                }
              });
          }
        });
      } else {
        this.backend.getAliasByName(this.sendForm.get('address').value.replace('@', ''), (alias_status, alias_data) => {
          this.ngZone.run(() => {
            if (alias_status === false) {
              this.ngZone.run(() => {
                this.sendForm.get('address').setErrors({'alias_not_valid': true});
              });
            } else {
              this.backend.sendMoney(
                this.currentWalletId,
                alias_data.address, // this.sendForm.get('address').value,
                this.sendForm.get('amount').value,
                this.sendForm.get('fee').value,
                this.sendForm.get('mixin').value,
                this.sendForm.get('comment').value,
                this.sendForm.get('hide').value,
                (send_status) => {
                  if (send_status) {
                    this.modalService.prepareModal('success', 'SEND.SUCCESS_SENT');
                    this.variablesService.currentWallet.send_data = {address: null, amount: null, comment: null, mixin: null, fee: null, hide: null};
                    this.sendForm.reset({address: null, amount: null, comment: null, mixin: 10, fee: this.variablesService.default_fee, hide: false});
                  }
                });
            }
          });
        });
      }
    }
  }

  toggleOptions() {
    this.additionalOptions = !this.additionalOptions;
  }

  ngOnDestroy() {
    this.parentRouting.unsubscribe();
    this.variablesService.currentWallet.send_data = {
      address: this.sendForm.get('address').value,
      amount: this.sendForm.get('amount').value,
      comment: this.sendForm.get('comment').value,
      mixin: this.sendForm.get('mixin').value,
      fee: this.sendForm.get('fee').value,
      hide: this.sendForm.get('hide').value
    };
  }

}
