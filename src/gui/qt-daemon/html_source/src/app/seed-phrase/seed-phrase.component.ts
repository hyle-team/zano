import {Component, NgZone, OnDestroy, OnInit} from '@angular/core';
import {Location} from '@angular/common';
import {BackendService} from '../_helpers/services/backend.service';
import {ActivatedRoute, Router} from '@angular/router';
import {VariablesService} from '../_helpers/services/variables.service';
import {ModalService} from '../_helpers/services/modal.service';
import { FormControl, FormGroup, Validators } from '@angular/forms';

@Component({
  selector: 'app-seed-phrase',
  templateUrl: './seed-phrase.component.html',
  styleUrls: ['./seed-phrase.component.scss']
})
export class SeedPhraseComponent implements OnInit, OnDestroy {

  queryRouting;
  seedPhrase = '';
  showSeed = false;
  wallet_id: number;
  seedPhraseCopied = false;
  progressWidth = '66%';

  detailsForm = new FormGroup({
    name: new FormControl('', [
      Validators.required,
      (g: FormControl) => {
        for (let i = 0; i < this.variablesService.wallets.length; i++) {
          if (g.value === this.variablesService.wallets[i].name) {
            if (
              this.variablesService.wallets[i].wallet_id ===
              this.variablesService.currentWallet.wallet_id
            ) {
              return { same: true };
            } else {
              return { duplicate: true };
            }
          }
        }
        return null;
      },
    ]),
    path: new FormControl(''),
  });

  seedPhraseForm = new FormGroup(
    {
      password: new FormControl(
        '',
        Validators.pattern(this.variablesService.pattern)
      ),
      confirmPassword: new FormControl(
        '',
        Validators.pattern(this.variablesService.pattern)
      ),
    },
    { validators: this.checkPasswords }
  );

  checkPasswords(group: FormGroup) {
    const pass = group.controls.password.value;
    const confirmPass = group.controls.confirmPassword.value;

    return pass === confirmPass ? null : { notSame: true };
  }

  constructor(
    private route: ActivatedRoute,
    private router: Router,
    private location: Location,
    private backend: BackendService,
    public variablesService: VariablesService,
    private modalService: ModalService,
    private ngZone: NgZone
  ) {}

  ngOnInit() {
    this.showSeed = false;
    this.getWalletId();
    this.setWalletInfoNamePath();
  }

  private setWalletInfoNamePath() {
    this.detailsForm
      .get('name')
      .setValue(this.variablesService.opening_wallet.name);
    this.detailsForm
      .get('path')
      .setValue(this.variablesService.opening_wallet.path);
  }

  private getWalletId() {
    this.queryRouting = this.route.queryParams.subscribe(params => {
      if (params.wallet_id) {
        this.wallet_id = params.wallet_id;
      }
    });
  }

  runWallet() {
    let exists = false;
    this.variablesService.wallets.forEach((wallet) => {
      if (wallet.address === this.variablesService.opening_wallet.address) {
        exists = true;
      }
    });
    if (!exists) {
      this.backend.runWallet(this.wallet_id, (run_status, run_data) => {
        if (run_status) {
          this.variablesService.wallets.push(this.variablesService.opening_wallet);
          if (this.variablesService.appPass) {
            this.backend.storeSecureAppData();
          }
          this.ngZone.run(() => {
            this.router.navigate(['/wallet/' + this.wallet_id]);
          });
        } else {
          console.log(run_data['error_code']);
        }
      });
    } else {
      this.variablesService.opening_wallet = null;
      this.modalService.prepareModal('error', 'OPEN_WALLET.WITH_ADDRESS_ALREADY_OPEN');
      this.backend.closeWallet(this.wallet_id, () => {
        this.ngZone.run(() => {
          this.router.navigate(['/']);
        });
      });
    }
  }

  copySeedPhrase() {
    this.backend.setClipboard(this.seedPhrase, () => {
      this.ngZone.run(() => {
        this.seedPhraseCopied = true;
      });
    });
  }

  back() {
    this.location.back();
  }

  showSeedPhrase() {
    this.showSeed = true;
    this.progressWidth = '100%';
  }

  onSubmitSeed() {
    if (this.seedPhraseForm.valid) {
      this.showSeedPhrase();
      const wallet_id = this.wallet_id;
      const seed_password = this.seedPhraseForm.controls.password.value;
      this.backend.getSmartWalletInfo(
        { wallet_id, seed_password },
        (status, data) => {
          if (data.hasOwnProperty('seed_phrase')) {
            this.ngZone.run(() => {
              this.seedPhrase = data['seed_phrase'].trim();
            });
          }
        }
      );
    }
  }

  ngOnDestroy() {
    this.queryRouting.unsubscribe();
  }
}
