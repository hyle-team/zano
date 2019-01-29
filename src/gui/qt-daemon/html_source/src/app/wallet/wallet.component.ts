import {Component, OnInit, OnDestroy, NgZone} from '@angular/core';
import {ActivatedRoute, Router} from '@angular/router';
import {VariablesService} from '../_helpers/services/variables.service';
import {BackendService} from '../_helpers/services/backend.service';
import {TranslateService} from '@ngx-translate/core';
import {IntToMoneyPipe} from '../_helpers/pipes/int-to-money.pipe';
import {BigNumber} from 'bignumber.js';

@Component({
  selector: 'app-wallet',
  templateUrl: './wallet.component.html',
  styleUrls: ['./wallet.component.scss']
})
export class WalletComponent implements OnInit, OnDestroy {
  subRouting;
  walletID;
  tabs = [
    {
      title: 'WALLET.TABS.HISTORY',
      icon: 'history',
      link: '/history',
      indicator: false,
      active: true
    },
    {
      title: 'WALLET.TABS.SEND',
      icon: 'send',
      link: '/send',
      indicator: false,
      active: false
    },
    {
      title: 'WALLET.TABS.RECEIVE',
      icon: 'receive',
      link: '/receive',
      indicator: false,
      active: false
    },
    {
      title: 'WALLET.TABS.CONTRACTS',
      icon: 'contracts',
      link: '/contracts',
      indicator: 1,
      active: false
    },
    /*{
      title: 'WALLET.TABS.MESSAGES',
      icon: 'messages',
      link: '/messages',
      indicator: 32,
      active: false
    },*/
    {
      title: 'WALLET.TABS.STAKING',
      icon: 'staking',
      link: '/staking',
      indicator: false,
      active: false
    }
  ];

  constructor(
    private route: ActivatedRoute,
    private router: Router,
    private backend: BackendService,
    private variablesService: VariablesService,
    private ngZone: NgZone,
    private translate: TranslateService,
    private intToMoneyPipe: IntToMoneyPipe
  ) {
  }

  ngOnInit() {
    this.subRouting = this.route.params.subscribe(params => {
      this.walletID = +params['id'];
      this.variablesService.setCurrentWallet(this.walletID);
      for (let i = 0; i < this.tabs.length; i++) {
        this.tabs[i].active = (this.tabs[i].link === '/' + this.route.snapshot.firstChild.url[0].path);
      }
    });
  }

  changeTab(index) {
    if ((this.tabs[index].link === '/send' || this.tabs[index].link === '/contracts' || this.tabs[index].link === '/staking') && this.variablesService.daemon_state !== 2) {
      return;
    }
    this.tabs.forEach((tab) => {
      tab.active = false;
    });
    this.tabs[index].active = true;
    this.router.navigate(['wallet/' + this.walletID + this.tabs[index].link]);
  }

  copyAddress() {
    this.backend.setClipboard(this.variablesService.currentWallet.address);
  }

  getTooltip() {
    const tooltip = document.createElement('div');
    const available = document.createElement('span');
    available.setAttribute('class', 'available');
    available.innerHTML = this.translate.instant('WALLET.AVAILABLE_BALANCE', {available: this.intToMoneyPipe.transform(this.variablesService.currentWallet.unlocked_balance), currency: this.variablesService.defaultCurrency});
    tooltip.appendChild(available);
    const locked = document.createElement('span');
    locked.setAttribute('class', 'locked');
    locked.innerHTML = this.translate.instant('WALLET.LOCKED_BALANCE', {locked: this.intToMoneyPipe.transform(this.variablesService.currentWallet.balance.minus(this.variablesService.currentWallet.unlocked_balance)), currency: this.variablesService.defaultCurrency});
    tooltip.appendChild(locked);
    const link = document.createElement('span');
    link.setAttribute('class', 'link');
    link.innerHTML = this.translate.instant('WALLET.LOCKED_BALANCE_LINK');
    link.addEventListener('click', () => {
      this.openInBrowser('docs.zano.org/docs/locked-balance');
    });
    tooltip.appendChild(link);
    return tooltip;
  }

  openInBrowser(link) {
    this.backend.openUrlInBrowser(link);
  }

  ngOnDestroy() {
    this.subRouting.unsubscribe();
  }

}
