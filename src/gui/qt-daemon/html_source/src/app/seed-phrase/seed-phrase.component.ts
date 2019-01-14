import {Component, NgZone, OnDestroy, OnInit} from '@angular/core';
import {BackendService} from '../_helpers/services/backend.service';
import {ActivatedRoute, Router} from '@angular/router';
import {VariablesService} from '../_helpers/services/variables.service';

@Component({
  selector: 'app-seed-phrase',
  templateUrl: './seed-phrase.component.html',
  styleUrls: ['./seed-phrase.component.scss']
})
export class SeedPhraseComponent implements OnInit, OnDestroy {

  queryRouting;
  seedPhrase = '';
  wallet_id: number;

  constructor(
    private route: ActivatedRoute,
    private router: Router,
    private backend: BackendService,
    private variablesService: VariablesService,
    private ngZone: NgZone
  ) {}

  ngOnInit() {
    this.queryRouting = this.route.queryParams.subscribe(params => {
      if (params.wallet_id) {
        this.wallet_id = params.wallet_id;
        this.backend.getSmartSafeInfo(params.wallet_id, (status, data) => {
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
          this.backend.storeSecureAppData((status, data) => {
            console.log('Store App Data', status, data);
          });
          this.ngZone.run(() => {
            this.router.navigate(['/wallet/' + this.wallet_id]);
          });
        } else {
          console.log(run_data['error_code']);
        }
      });
    } else {
      this.variablesService.opening_wallet = null;
      this.backend.closeWallet(this.wallet_id, (close_status, close_data) => {
        console.log(close_status, close_data);
        this.ngZone.run(() => {
          this.router.navigate(['/']);
        });
      });
    }
  }

  ngOnDestroy() {
    this.queryRouting.unsubscribe();
  }

}
