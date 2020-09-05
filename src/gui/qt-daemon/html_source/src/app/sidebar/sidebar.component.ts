import {Component, NgZone, OnInit, OnDestroy} from '@angular/core';
import {ActivatedRoute, NavigationStart, Router} from '@angular/router';
import {VariablesService} from '../_helpers/services/variables.service';
import {BackendService} from '../_helpers/services/backend.service';
import { ModalService } from '../_helpers/services/modal.service';
import {AUDITABLE_WALLET_HELP_PAGE} from '../_shared/constants';

import icons from '../../assets/icons/icons.json';

@Component({
  selector: 'app-sidebar',
  templateUrl: './sidebar.component.html',
  styleUrls: ['./sidebar.component.scss']
})
export class SidebarComponent implements OnInit, OnDestroy {
  walletSubRouting;

  walletActive: number;

  contacts = icons.contacts;
  settings = icons.settings;
  exit = icons.exit;

  isModalDialogVisible = false;
  closeWalletId: number;

  menuItemHovered: boolean;

  constructor(
    private route: ActivatedRoute,
    private router: Router,
    public variablesService: VariablesService,
    private backend: BackendService,
    private modal: ModalService,
    private ngZone: NgZone
  ) {}

  ngOnInit() {
    if (this.router.url.indexOf('/wallet/') !== -1) {
      const localPathArr = this.router.url.split('/');
      if (localPathArr.length >= 3) {
        this.walletActive = parseInt(localPathArr[2], 10);
      }
    } else if (this.router.url.indexOf('/details') !== -1) {
      this.walletActive = this.variablesService.currentWallet.wallet_id;
    } else {
      this.walletActive = null;
    }

    this.walletSubRouting = this.router.events.subscribe((event) => {
      if (event instanceof NavigationStart) {
        if (event.url.indexOf('/wallet/') !== -1) {
          const localPathArr = event.url.split('/');
          if (localPathArr.length >= 3) {
            this.walletActive = parseInt(localPathArr[2], 10);
          }
        } else if (event.url.indexOf('/details') !== -1) {
          this.walletActive = this.variablesService.currentWallet.wallet_id;
        } else {
          this.walletActive = null;
        }
      }
    });
  }

  goMainPage() {
    if (this.route.snapshot.queryParams && this.route.snapshot.queryParams.prevUrl === 'login') {
      this.ngZone.run(() => {
        this.router.navigate(['/'], {queryParams: {prevUrl: 'login'}});
      });
    } else {
      this.ngZone.run(() => {
        this.router.navigate(['/']);
      });
    }

  };

  contactsRoute() {
    if (this.variablesService.appPass) {
      this.router.navigate(['/contacts']);
    } else {
      this.modal.prepareModal(
        'error',
        'CONTACTS.FORM_ERRORS.SET_MASTER_PASSWORD'
      );
    }
  }

  showDialog(wallet_id) {
    this.isModalDialogVisible = true;
    this.closeWalletId = wallet_id;
  }

  confirmed(confirmed: boolean) {
    if (confirmed) {
      this.closeWallet(this.closeWalletId);
    }
    this.isModalDialogVisible = false;
  }

  closeWallet(wallet_id) {
    this.backend.closeWallet(wallet_id, () => {
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

  getUpdate() {
    this.backend.openUrlInBrowser('zano.org/downloads.html');
  }
  goToAuditableWalletHelpPage(e) {
    e.preventDefault();
    this.backend.openUrlInBrowser(AUDITABLE_WALLET_HELP_PAGE);
  }

  logOut() {
    this.variablesService.stopCountdown();
    this.variablesService.appLogin = false;
    this.variablesService.appPass = '';
    this.ngZone.run(() => {
      this.router.navigate(['/login'], {queryParams: {type: 'auth'}});
    });
  }

  ngOnDestroy() {
    this.walletSubRouting.unsubscribe();
  }
}
