import { Injectable, NgZone } from '@angular/core';
import { Router } from '@angular/router';

import { BackendService } from './backend.service';
import { VariablesService } from './variables.service';

@Injectable()
export class WalletService {

  constructor(
    private backend: BackendService,
    private variablesService: VariablesService,
    private router: Router,
    private ngZone: NgZone,
  ) { }

  closeCurrent() {
    this.backend.closeWallet(this.variablesService.currentWallet.wallet_id, () => {
      for (let i = this.variablesService.wallets.length - 1; i >= 0; i--) {
        if (this.variablesService.wallets[i].wallet_id === this.variablesService.currentWallet.wallet_id) {
          this.variablesService.wallets.splice(i, 1);
        }
      }
      this.ngZone.run(() => {
        if (this.variablesService.wallets.length) {
          this.variablesService.currentWallet = this.variablesService.wallets[0];
          this.router.navigate(['/wallet/' + this.variablesService.currentWallet.wallet_id]);
        } else {
          this.router.navigate(['/']);
        }
      });
      if (this.variablesService.appPass) {
        this.backend.storeSecureAppData();
      }
    });
  }
}
