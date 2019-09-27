import {Component, NgZone, OnInit, OnDestroy} from '@angular/core';
import {ActivatedRoute, NavigationStart, Router} from '@angular/router';
import {VariablesService} from '../_helpers/services/variables.service';
import {BackendService} from '../_helpers/services/backend.service';
import { ModalService } from '../_helpers/services/modal.service';
<<<<<<< HEAD

import icons from '../../assets/icons/icons.json';
=======
>>>>>>> contact service

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

  getUpdate() {
    this.backend.openUrlInBrowser('zano.org/downloads.html');
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
