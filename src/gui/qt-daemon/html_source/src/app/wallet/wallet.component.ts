import { Component, OnInit, OnDestroy, NgZone, ViewChild, ElementRef } from '@angular/core';
import { ActivatedRoute, Router, RoutesRecognized } from '@angular/router';
import { VariablesService } from '../_helpers/services/variables.service';
import { BackendService } from '../_helpers/services/backend.service';
import { TranslateService } from '@ngx-translate/core';
import { IntToMoneyPipe } from '../_helpers/pipes/int-to-money.pipe';
import { Subscription } from 'rxjs';

import icons from '../../assets/icons/icons.json';
import { PaginationService } from '../_helpers/services/pagination.service';

@Component({
  selector: 'app-wallet',
  templateUrl: './wallet.component.html',
  styleUrls: ['./wallet.component.scss']
})
export class WalletComponent implements OnInit, OnDestroy {
  subRouting1;
  subRouting2;
  queryRouting;
  walletID;
  copyAnimation = false;
  copyAnimationTimeout;
  balanceTooltip;
  activeTab = 'history';

  public currentPage = 1;

  @ViewChild('scrolledContent') private scrolledContent: ElementRef;

  tabs = [
    {
      title: 'WALLET.TABS.HISTORY',
      icon: 'history',
      link: '/history',
      indicator: false,
      active: true,
      animated: icons.history,
      itemHovered: false
    },
    {
      title: 'WALLET.TABS.SEND',
      icon: 'send',
      link: '/send',
      indicator: false,
      active: false,
      animated: icons.send,
      itemHovered: false
    },
    {
      title: 'WALLET.TABS.RECEIVE',
      icon: 'receive',
      link: '/receive',
      indicator: false,
      active: false,
      animated: icons.receive,
      itemHovered: false
    },
    {
      title: 'WALLET.TABS.CONTRACTS',
      icon: 'contracts',
      link: '/contracts',
      indicator: 1,
      active: false,
      animated: icons.contracts,
      itemHovered: false
    },
    /*{
      title: 'WALLET.TABS.MESSAGES',
      icon: 'messages',
      link: '/messages',
      indicator: 32,
      active: false,
      animated: icons.messages,
      itemHovered: false
    },*/
    {
      title: 'WALLET.TABS.STAKING',
      icon: 'staking',
      link: '/staking',
      indicator: false,
      active: false,
      animated: icons.staking,
      itemHovered: false
    }
  ];
  aliasSubscription: Subscription;

  constructor(
    private route: ActivatedRoute,
    private router: Router,
    private backend: BackendService,
    public variablesService: VariablesService,
    private ngZone: NgZone,
    private translate: TranslateService,
    private intToMoneyPipe: IntToMoneyPipe,
    private pagination: PaginationService
  ) { }

  ngOnInit() {
    this.subRouting1 = this.route.params.subscribe(params => {
      this.walletID = +params['id'];
      this.variablesService.setCurrentWallet(this.walletID);
      this.scrolledContent.nativeElement.scrollTop = 0;
      clearTimeout(this.copyAnimationTimeout);
      this.copyAnimation = false;
    });
    this.subRouting2 = this.router.events.subscribe(val => {
      if (val instanceof RoutesRecognized) {
        this.activeTab = val.urlAfterRedirects.split('/').pop();
        if (val.state.root.firstChild && val.state.root.firstChild.firstChild) {
          for (let i = 0; i < this.tabs.length; i++) {
            this.tabs[i].active = (this.tabs[i].link === '/' + val.state.root.firstChild.firstChild.url[0].path);
          }
        }
      }
    });
    this.queryRouting = this.route.queryParams.subscribe(params => {
      if (params.send) {
        this.tabs.forEach((tab, index) => {
          if (tab.link === '/send') {
            this.changeTab(index);
          }
        });
      }
    });
    if (this.variablesService.currentWallet.alias.hasOwnProperty('name')) {
      this.variablesService.currentWallet.wakeAlias = false;
    }
    this.aliasSubscription = this.variablesService.getAliasChangedEvent.subscribe(() => {
      if (this.variablesService.currentWallet.alias.hasOwnProperty('name')) {
        this.variablesService.currentWallet.wakeAlias = false;
      }
    });
  }

  changeTab(index) {
    if ((this.tabs[index].link === '/send' || this.tabs[index].link === '/contracts' || this.tabs[index].link === '/staking') && (this.variablesService.daemon_state !== 2 || !this.variablesService.currentWallet.loaded)) {
      return;
    }
    this.tabs.forEach((tab) => {
      tab.active = false;
    });
    this.tabs[index].active = true;
    this.ngZone.run(() => {
      this.scrolledContent.nativeElement.scrollTop = 0;
      this.router.navigate(['wallet/' + this.walletID + this.tabs[index].link]);
    });
  }

  itemHovered(index, state: boolean) {
    this.tabs[index].itemHovered = state;
  }

  copyAddress() {
    this.backend.setClipboard(this.variablesService.currentWallet.address);
    this.copyAnimation = true;
    this.copyAnimationTimeout = window.setTimeout(() => {
      this.copyAnimation = false;
    }, 2000);
  }

  getTooltip() {
    this.balanceTooltip = document.createElement('div');
    const available = document.createElement('span');
    available.setAttribute('class', 'available');
    available.innerHTML = this.translate.instant('WALLET.AVAILABLE_BALANCE', { available: this.intToMoneyPipe.transform(this.variablesService.currentWallet.unlocked_balance), currency: this.variablesService.defaultCurrency });
    this.balanceTooltip.appendChild(available);
    const locked = document.createElement('span');
    locked.setAttribute('class', 'locked');
    locked.innerHTML = this.translate.instant('WALLET.LOCKED_BALANCE', { locked: this.intToMoneyPipe.transform(this.variablesService.currentWallet.balance.minus(this.variablesService.currentWallet.unlocked_balance)), currency: this.variablesService.defaultCurrency });
    this.balanceTooltip.appendChild(locked);
    const link = document.createElement('span');
    link.setAttribute('class', 'link');
    link.innerHTML = this.translate.instant('WALLET.LOCKED_BALANCE_LINK');
    link.addEventListener('click', () => {
      this.openInBrowser('docs.zano.org/docs/locked-balance');
    });
    this.balanceTooltip.appendChild(link);
    return this.balanceTooltip;
  }

  onHideTooltip() {
    this.balanceTooltip = null;
  }

  openInBrowser(link) {
    this.backend.openUrlInBrowser(link);
  }

  public setPage(pageNumber: number) {
    if (pageNumber === this.variablesService.currentWallet.currentPage) {
      return;
    }
    this.variablesService.currentWallet.currentPage = pageNumber;
    this.backend.getRecentTransfers(
      this.walletID,
      (this.variablesService.currentWallet.currentPage - 1) * this.variablesService.count,
      this.variablesService.count, (status, data) => {
        if (status && data.total_history_items) {
          this.variablesService.currentWallet.history.splice(0, this.variablesService.currentWallet.history.length);
          this.ngZone.run(() => {
            this.pagination.paginate(this.variablesService.currentWallet.currentPage);
            if (data.history.length !== 0) {
              this.variablesService.currentWallet.restore = false;
              this.variablesService.currentWallet.total_history_item = data.total_history_items;
              this.variablesService.currentWallet.prepareHistory(data.history);
            }
          });
        }
      });
  }

  ngOnDestroy() {
    this.subRouting1.unsubscribe();
    this.subRouting2.unsubscribe();
    this.queryRouting.unsubscribe();
    this.aliasSubscription.unsubscribe();
    clearTimeout(this.copyAnimationTimeout);
  }

}
