import {Component, NgZone, OnInit} from '@angular/core';
import {FormGroup, FormControl, Validators} from '@angular/forms';
import {BackendService} from '../_helpers/services/backend.service';
import {VariablesService} from '../_helpers/services/variables.service';
import {Router} from '@angular/router';
import {Wallet} from '../_helpers/models/wallet.model';

@Component({
  selector: 'app-create-wallet',
  templateUrl: './create-wallet.component.html',
  styleUrls: ['./create-wallet.component.scss']
})
export class CreateWalletComponent implements OnInit {

  createForm = new FormGroup({
    name: new FormControl('', [Validators.required, (g: FormControl) => {
      for (let i = 0; i < this.variablesService.wallets.length; i++) {
        if (g.value === this.variablesService.wallets[i].name) {
          return {'duplicate': true};
        }
      }
      return null;
    }]),
    password: new FormControl(''),
    confirm: new FormControl('')
  }, function (g: FormGroup) {
    return g.get('password').value === g.get('confirm').value ? null : {'confirm_mismatch': true};
  });

  wallet = {
    id: ''
  };

  walletSaved = false;

  constructor(
    private router: Router,
    private backend: BackendService,
    private variablesService: VariablesService,
    private ngZone: NgZone
  ) {
  }

  ngOnInit() {
  }

  createWallet() {
    this.router.navigate(['/seed-phrase'], {queryParams: {wallet_id: this.wallet.id}});
  }

  saveWallet() {
    if (this.createForm.valid) {
      this.backend.saveFileDialog('Save the wallet file.', '*', this.variablesService.settings.default_path, (file_status, file_data) => {
        if (file_status) {
          this.variablesService.settings.default_path = file_data.path.substr(0, file_data.path.lastIndexOf('/'));
          this.backend.generateWallet(file_data.path, this.createForm.get('password').value, (generate_status, generate_data, errorCode) => {
            if (generate_status) {
              this.wallet.id = generate_data.wallet_id;
              this.variablesService.opening_wallet = new Wallet(
                generate_data.wallet_id,
                this.createForm.get('name').value,
                this.createForm.get('password').value,
                generate_data['wi'].path,
                generate_data['wi'].address,
                generate_data['wi'].balance,
                generate_data['wi'].unlocked_balance,
                generate_data['wi'].mined_total,
                generate_data['wi'].tracking_hey
              );
              this.ngZone.run(() => {
                this.walletSaved = true;
              });
            } else {
              if (errorCode && errorCode === 'ALREADY_EXISTS') {
                alert('You cannot record a file on top of another file');
              } else {
                alert('You cannot save a safe file to the system partition');
              }
            }
          });
        }
      });
    }
  }

}
