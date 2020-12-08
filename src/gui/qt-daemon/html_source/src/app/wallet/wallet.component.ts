import {
  Component,
  OnInit,
  OnDestroy,
  NgZone,
  ViewChild,
  ElementRef,
} from '@angular/core';
import { ActivatedRoute, Router, RoutesRecognized } from '@angular/router';
import { VariablesService } from '../_helpers/services/variables.service';
import { BackendService } from '../_helpers/services/backend.service';
import { TranslateService } from '@ngx-translate/core';
import { IntToMoneyPipe } from '../_helpers/pipes/int-to-money.pipe';
import { Subscription } from 'rxjs';
import { LOCKED_BALANCE_HELP_PAGE } from '../_shared/constants';

import icons from '../../assets/icons/icons.json';
import { PaginationService } from '../_helpers/services/pagination.service';
import { PaginationStore } from '../_helpers/services/pagination.store';
import { Store, Sync } from 'store';
import { Wallet } from '../_helpers/models/wallet.model';
import { distinctUntilChanged, filter } from 'rxjs/operators';

@Component({
  selector: 'app-wallet',
  templateUrl: './wallet.component.html',
  styleUrls: ['./wallet.component.scss'],
})
export class WalletComponent implements OnInit, OnDestroy {
  subRouting1;
  subRouting2;
  queryRouting;
  walletID;
  copyAnimation = false;
  copyAnimationTimeout;
  balanceTooltip;
  walletLoaded;
  activeTab = 'history';
  public mining = false;
  public currentPage = 1;
  wallet: Wallet;
  sync_started = false;
  stop_paginate = false;

  @ViewChild('scrolledContent') private scrolledContent: ElementRef;

  tabs = [
    {
      title: 'WALLET.TABS.HISTORY',
      icon: 'history',
      link: '/history',
      indicator: false,
      active: true,
      animated: icons.history,
      itemHovered: false,
    },
    {
      title: 'WALLET.TABS.SEND',
      icon: 'send',
      link: '/send',
      indicator: false,
      active: false,
      animated: icons.send,
      itemHovered: false,
    },
    {
      title: 'WALLET.TABS.RECEIVE',
      icon: 'receive',
      link: '/receive',
      indicator: false,
      active: false,
      animated: icons.receive,
      itemHovered: false,
    },
    {
      title: 'WALLET.TABS.CONTRACTS',
      icon: 'contracts',
      link: '/contracts',
      indicator: 1,
      active: false,
      animated: icons.contracts,
      itemHovered: false,
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
      itemHovered: false,
    },
  ];
  aliasSubscription: Subscription;
  walletsSubscription: Subscription;

  constructor(
    private route: ActivatedRoute,
    private router: Router,
    private backend: BackendService,
    public variablesService: VariablesService,
    private ngZone: NgZone,
    private translate: TranslateService,
    private intToMoneyPipe: IntToMoneyPipe,
    private pagination: PaginationService,
    private paginationStore: PaginationStore,
    private store: Store
  ) {}

  ngOnInit() {
    this.subRouting1 = this.route.params.subscribe((params) => {
      // set current wallet only by user click to avoid after sync show synchronized data
      this.walletID = +params['id'];
      this.walletLoaded = this.variablesService.getWallet(this.walletID).loaded;
      this.variablesService.setCurrentWallet(this.walletID);
      this.walletsSubscription = this.store
        .select('sync')
        .pipe(filter(Boolean), distinctUntilChanged())
        .subscribe((value) => {
          const data = value.filter(
            (item: Sync) => item.wallet_id === this.walletID
          )[0];
          if (data && !data.sync) {
            let in_progress;
            const values = this.store.value.sync;
            if (values && values.length) {
              in_progress = values.filter((item) => item.sync);
              this.variablesService.sync_started = !!(
                in_progress && in_progress.length
              );
              if (!in_progress) {
                this.variablesService.sync_started = false;
              }
            } else {
              this.variablesService.sync_started = false;
            }
          }
          let restore = false;
          if (
            this.variablesService.after_sync_request.hasOwnProperty(
              this.walletID
            )
          ) {
            restore = this.variablesService.after_sync_request[this.walletID];
          }
          if (
            !this.variablesService.sync_started &&
            restore &&
            this.walletID === (data && data.wallet_id)
          ) {
            this.wallet = this.variablesService.getNotLoadedWallet();
            if (this.wallet) {
              this.tick();
            }
            // if this is was restore wallet and it was selected on moment when sync completed
            this.getRecentTransfers();
            this.variablesService.after_sync_request[this.walletID] = false;
          }
        });
      let after_sync_request = false;
      if (
        this.variablesService.after_sync_request.hasOwnProperty(this.walletID)
      ) {
        after_sync_request = this.variablesService.after_sync_request[
          this.walletID
        ];
      }
      if (after_sync_request && !this.variablesService.sync_started) {
        // if user click on the wallet at the first time after restore.
        this.getRecentTransfers();
      }

      if (this.variablesService.stop_paginate.hasOwnProperty(this.walletID)) {
        this.stop_paginate = this.variablesService.stop_paginate[this.walletID];
      } else {
        this.stop_paginate = false;
      }
      // this will hide pagination a bit earlier
      this.wallet = this.variablesService.getNotLoadedWallet();
      if (this.wallet) {
        this.tick();
      }

      this.scrolledContent.nativeElement.scrollTop = 0;
      clearTimeout(this.copyAnimationTimeout);
      this.copyAnimation = false;
      this.mining = this.variablesService.currentWallet.exclude_mining_txs;

      if (this.variablesService.wallets.length === 1) {
        this.walletID = +params['id'];
        this.variablesService.setCurrentWallet(this.walletID);
      }
    });
    this.subRouting2 = this.router.events.subscribe((val) => {
      if (val instanceof RoutesRecognized) {
        this.activeTab = val.urlAfterRedirects
          .replace('?sidenav=true', '')
          .split('/')
          .pop();
        if (val.state.root.firstChild && val.state.root.firstChild.firstChild) {
          for (let i = 0; i < this.tabs.length; i++) {
            this.tabs[i].active =
              this.tabs[i].link ===
              '/' + val.state.root.firstChild.firstChild.url[0].path;
          }
        }
      }
    });
    this.queryRouting = this.route.queryParams.subscribe((params) => {
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
    this.aliasSubscription = this.variablesService.getAliasChangedEvent.subscribe(
      () => {
        if (this.variablesService.currentWallet.alias.hasOwnProperty('name')) {
          this.variablesService.currentWallet.wakeAlias = false;
        }
      }
    );
    this.updateWalletStatus();
  }
  resetPaginationValues() {
    this.ngZone.run(() => {
      const total_history_item = this.variablesService.currentWallet
        .total_history_item;
      const count = this.variablesService.count;
      this.variablesService.currentWallet.totalPages = Math.ceil(
        total_history_item / count
      );
      this.variablesService.currentWallet.exclude_mining_txs = this.mining;
      this.variablesService.currentWallet.currentPage = 1;

      if (!this.variablesService.currentWallet.totalPages) {
        this.variablesService.currentWallet.totalPages = 1;
      }
      this.variablesService.currentWallet.totalPages >
      this.variablesService.maxPages
        ? (this.variablesService.currentWallet.pages = new Array(5)
            .fill(1)
            .map((value, index) => value + index))
        : (this.variablesService.currentWallet.pages = new Array(
            this.variablesService.currentWallet.totalPages
          )
            .fill(1)
            .map((value, index) => value + index));
    });
  }

  changeTab(index) {
    if (
      ((this.tabs[index].link === '/send' ||
        this.tabs[index].link === '/contracts' ||
        this.tabs[index].link === '/staking') &&
        (this.variablesService.daemon_state !== 2 ||
          !this.variablesService.currentWallet.loaded)) ||
      ((this.tabs[index].link === '/send' ||
        this.tabs[index].link === '/contracts') &&
        this.variablesService.currentWallet.is_watch_only &&
        this.variablesService.currentWallet.is_auditable)
    ) {
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
    available.innerHTML = this.translate.instant('WALLET.AVAILABLE_BALANCE', {
      available: this.intToMoneyPipe.transform(
        this.variablesService.currentWallet.unlocked_balance
      ),
      currency: this.variablesService.defaultCurrency,
    });
    this.balanceTooltip.appendChild(available);
    const locked = document.createElement('span');
    locked.setAttribute('class', 'locked');
    locked.innerHTML = this.translate.instant('WALLET.LOCKED_BALANCE', {
      locked: this.intToMoneyPipe.transform(
        this.variablesService.currentWallet.balance.minus(
          this.variablesService.currentWallet.unlocked_balance
        )
      ),
      currency: this.variablesService.defaultCurrency,
    });
    this.balanceTooltip.appendChild(locked);
    const link = document.createElement('span');
    link.setAttribute('class', 'link');
    link.innerHTML = this.translate.instant('WALLET.LOCKED_BALANCE_LINK');
    link.addEventListener('click', () => {
      this.openInBrowser(LOCKED_BALANCE_HELP_PAGE);
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
    // this is will allow pagination for wallets that was open from existed wallets'
    if (
      this.variablesService.currentWallet.open_from_exist &&
      !this.variablesService.currentWallet.updated
    ) {
      this.variablesService.get_recent_transfers = false;
      this.variablesService.currentWallet.updated = true;
    }
    if (pageNumber === this.variablesService.currentWallet.currentPage) {
      return;
    }
    // if not running get_recent_transfers callback
    if (!this.variablesService.get_recent_transfers) {
      this.variablesService.currentWallet.currentPage = pageNumber;
    }
    if (!this.variablesService.get_recent_transfers) {
      this.getRecentTransfers();
    }
  }

  toggleMiningTransactions() {
    if (!this.variablesService.sync_started && !this.wallet) {
      const value = this.paginationStore.value;
      if (!value) {
        this.paginationStore.setPage(1, 0, this.walletID); // add back page for the first page
      } else {
        const pages = value.filter((item) => item.walletID === this.walletID);
        if (!pages.length) {
          this.paginationStore.setPage(1, 0, this.walletID); // add back page for the first page
        }
      }
      this.mining = !this.mining;
      this.resetPaginationValues();
      this.getRecentTransfers();
    }
  }

  tick() {
    const walletInterval = setInterval(() => {
      this.wallet = this.variablesService.getNotLoadedWallet();
      if (!this.wallet) {
        clearInterval(walletInterval);
      }
    }, 1000);
  }

  getRecentTransfers() {
    const offset = this.pagination.getOffset(this.walletID);
    const value = this.paginationStore.value;
    const pages = value
      ? value.filter((item) => item.walletID === this.walletID)
      : [];

    this.backend.getRecentTransfers(
      this.walletID,
      offset,
      this.variablesService.count,
      this.variablesService.currentWallet.exclude_mining_txs,
      (status, data) => {
        const isForward = this.paginationStore.isForward(
          pages,
          this.variablesService.currentWallet.currentPage
        );
        if (this.mining && isForward && pages && pages.length === 1) {
          this.variablesService.currentWallet.currentPage = 1; // set init page after navigation back
        }

        const history = data && data.history;
        this.variablesService.stop_paginate[this.walletID] =
          (history && history.length < this.variablesService.count) || !history;
        this.stop_paginate = this.variablesService.stop_paginate[this.walletID];
        if (!this.variablesService.stop_paginate[this.walletID]) {
          const page = this.variablesService.currentWallet.currentPage + 1;
          if (
            isForward &&
            this.mining &&
            history &&
            history.length === this.variablesService.count
          ) {
            this.paginationStore.setPage(
              page,
              data.last_item_index,
              this.walletID
            ); // add back page for current page
          }
        }

        this.pagination.calcPages(data);
        this.pagination.prepareHistory(data, status);

        this.ngZone.run(() => {
          this.variablesService.get_recent_transfers = false;
          if (
            this.variablesService.after_sync_request.hasOwnProperty(
              this.walletID
            )
          ) {
            // this is will complete get_recent_transfers request
            // this will switch of
            this.variablesService.after_sync_request[this.walletID] = false;
          }
        });
      }
    );
  }

  ngOnDestroy() {
    this.subRouting1.unsubscribe();
    this.subRouting2.unsubscribe();
    this.queryRouting.unsubscribe();
    this.aliasSubscription.unsubscribe();
    if (this.walletsSubscription) {
      this.walletsSubscription.unsubscribe();
    }
    clearTimeout(this.copyAnimationTimeout);
  }

  updateWalletStatus() {
    this.backend.eventSubscribe('wallet_sync_progress', (data) => {
      const wallet_id = data.wallet_id;
      if (wallet_id === this.walletID) {
        this.ngZone.run(() => {
          this.walletLoaded = false;
        });
      }
    });
    this.backend.eventSubscribe('update_wallet_status', (data) => {
      const wallet_state = data.wallet_state;
      const wallet_id = data.wallet_id;
      this.ngZone.run(() => {
        if (wallet_state === 2 && wallet_id === this.walletID) {
          this.walletLoaded =
            this.variablesService.getWallet(this.walletID) !== null &&
            this.variablesService.getWallet(this.walletID).loaded
              ? true
              : false;
        } else {
          this.walletLoaded = false;
        }
      });
    });
  }
}
