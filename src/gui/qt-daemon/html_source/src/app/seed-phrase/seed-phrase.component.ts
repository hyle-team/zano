import {Component, NgZone, OnDestroy, OnInit} from '@angular/core';
import {Location} from '@angular/common';
import {BackendService} from '../_helpers/services/backend.service';
import {ActivatedRoute, Router} from '@angular/router';
import {VariablesService} from '../_helpers/services/variables.service';
import {ModalService} from '../_helpers/services/modal.service';

@Component({
  selector: 'app-seed-phrase',
  templateUrl: './seed-phrase.component.html',
  styleUrls: ['./seed-phrase.component.scss']
})
export class SeedPhraseComponent implements OnInit, OnDestroy {

  queryRouting;
  seedPhrase = '';
  wallet_id: number;
  seedPhraseCopied = false;

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
    this.queryRouting = this.route.queryParams.subscribe(params => {
      if (params.wallet_id) {
        this.wallet_id = params.wallet_id;
        this.backend.getSmartWalletInfo(params.wallet_id, (status, data) => {
          if (data.hasOwnProperty('restore_key')) {
            this.ngZone.run(() => {
              this.seedPhrase = data['restore_key'].trim();
            });
          }
        });
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

  ngOnDestroy() {
    this.queryRouting.unsubscribe();
  }

}
