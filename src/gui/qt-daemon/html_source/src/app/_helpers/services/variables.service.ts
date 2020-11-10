import {Injectable, NgZone} from '@angular/core';
import {Wallet} from '../models/wallet.model';
import {Contact} from '../models/contact.model';
import {BehaviorSubject} from 'rxjs';
import {Idle} from 'idlejs/dist';
import {Router} from '@angular/router';
import {ContextMenuComponent, ContextMenuService} from 'ngx-contextmenu';
import {BigNumber} from 'bignumber.js';

@Injectable({
  providedIn: 'root'
})
export class VariablesService {
  public request_on_in = {};
  public stop_paginate = {};
  public sync_started = false;
  public digits = 12;
  public appPass = '';
  public appLogin = false;
  public moneyEquivalent = 0;
  public defaultTheme = 'dark';
  public defaultCurrency = 'ZANO';
  public opening_wallet: Wallet;
  public exp_med_ts = 0;
  public net_time_delta_median = 0;
  public height_app = 0;
  public height_max = 0;
  public downloaded = 0;
  public total = 0;
  public last_build_available = '';
  public last_build_displaymode = 0;
  public daemon_state = 3;
  public sync = {
    progress_value: 0,
    progress_value_text: '0'
  };
  public download = {
    progress_value: 0,
    progress_value_text: '0'
  };
  public get_recent_transfers = false; // avoid of execute function before collback complete
  public default_fee = '0.010000000000';
  public default_fee_big = new BigNumber('10000000000');

  public settings = {
    appLockTime: 15,
    appLog: 0,
    theme: '',
    scale: 10,
    language: 'en',
    default_path: '/',
    viewedContracts: [],
    notViewedContracts: [],
    wallets: []
  };

  public count = 40;
  public maxPages = 5;

  public testnet = false;
  public networkType = '';  // testnet of mainnet

  public wallets: Array<Wallet> = [];
  public currentWallet: Wallet;
  public selectWallet: number;
  public aliases: any = [];
  public aliasesChecked: any = {};
  public enableAliasSearch = false;
  public maxWalletNameLength = 25;
  public maxCommentLength = 255;
  public dataIsLoaded = false;

  public contacts: Array<Contact> = [];
  public newContact: Contact = {name: null, address: null, notes: null};

  public pattern = '^[a-zA-Z0-9_.\\\]\*\|\~\!\?\@\#\$\%\^\&\+\{\}\(\)\<\>\:\;\"\'\-\=\/\,\[\\\\]*$';
  public after_sync_request: any = {};
  getExpMedTsEvent = new BehaviorSubject(null);
  getHeightAppEvent = new BehaviorSubject(null);
  getHeightMaxEvent = new BehaviorSubject(null);
  getDownloadedAppEvent = new BehaviorSubject(null);
  getTotalEvent = new BehaviorSubject(null);
  getRefreshStackingEvent = new BehaviorSubject(null);
  getAliasChangedEvent = new BehaviorSubject(null);

  public idle = new Idle()
    .whenNotInteractive()
    .do(() => {
      if (this.appPass == '') {
        this.restartCountdown();
      } else {
        this.ngZone.run(() => {
        this.idle.stop();
        this.appPass = '';
        this.appLogin = false;
        this.router.navigate(['/login'], {queryParams: {type: 'auth'}});
      });
      }
    });

  public allContextMenu: ContextMenuComponent;
  public onlyCopyContextMenu: ContextMenuComponent;
  public pasteSelectContextMenu: ContextMenuComponent;

  constructor(private router: Router, private ngZone: NgZone, private contextMenuService: ContextMenuService) {
  }

  setExpMedTs(timestamp: number) {
    if (timestamp !== this.exp_med_ts) {
      this.exp_med_ts = timestamp;
      this.getExpMedTsEvent.next(timestamp);
    }
  }

  setHeightApp(height: number) {
    if (height !== this.height_app) {
      this.height_app = height;
      this.getHeightAppEvent.next(height);
    }
  }

  setHeightMax(height: number) {
    if (height !== this.height_max) {
      this.height_max = height;
      this.getHeightMaxEvent.next(height);
    }
  }

  setDownloadedBytes(bytes: number) {
    if (bytes !== this.downloaded) {
      this.downloaded = this.bytesToMb(bytes);
      this.getDownloadedAppEvent.next(bytes);
    }
  }

  setTotalBytes(bytes: number) {
    if (bytes !== this.total) {
      this.total = this.bytesToMb(bytes);
      this.getTotalEvent.next(bytes);
    }
  }

  setRefreshStacking(wallet_id: number) {
    this.getHeightAppEvent.next(wallet_id);
  }

  changeAliases() {
    this.getAliasChangedEvent.next(true);
  }

  setCurrentWallet(id): void {
    this.wallets.forEach((wallet) => {
      if (wallet.wallet_id === id) {
        this.currentWallet = wallet;
      }
    });
  }

  getWallet(id): Wallet {
    for (let i = 0; i < this.wallets.length; i++) {
      if (this.wallets[i].wallet_id === id) {
        return this.wallets[i];
      }
    }
    return null;
  }

  getNotLoadedWallet() {
    for (let i = 0; i < this.wallets.length; i++) {
      if (!this.wallets[i].loaded) {
        return this.wallets[i];
      }
    }
    return null;
  }

  startCountdown() {
    this.idle.within(this.settings.appLockTime).start();
  }

  stopCountdown() {
    this.idle.stop();
  }

  restartCountdown() {
    this.idle.within(this.settings.appLockTime).restart();
  }

  bytesToMb(bytes) {
    return Number((bytes / Math.pow(1024, 2)).toFixed(1));
  }

  public onContextMenu($event: MouseEvent): void {
    $event.target['contextSelectionStart'] = $event.target['selectionStart'];
    $event.target['contextSelectionEnd'] = $event.target['selectionEnd'];
    if ($event.target && ($event.target['nodeName'].toUpperCase() === 'TEXTAREA' || $event.target['nodeName'].toUpperCase() === 'INPUT') && !$event.target['readOnly']) {
      this.contextMenuService.show.next({
        contextMenu: this.allContextMenu,
        event: $event,
        item: $event.target,
      });
      $event.preventDefault();
      $event.stopPropagation();
    }
  }

  public onContextMenuOnlyCopy($event: MouseEvent, copyText?: string): void {
    this.contextMenuService.show.next({
      contextMenu: this.onlyCopyContextMenu,
      event: $event,
      item: copyText
    });
    $event.preventDefault();
    $event.stopPropagation();
  }

  public onContextMenuPasteSelect($event: MouseEvent): void {
    $event.target['contextSelectionStart'] = $event.target['selectionStart'];
    $event.target['contextSelectionEnd'] = $event.target['selectionEnd'];

    console.warn($event.target);
    console.warn($event.target['disabled']);


    if ($event.target && ($event.target['nodeName'].toUpperCase() === 'TEXTAREA' || $event.target['nodeName'].toUpperCase() === 'INPUT') && !$event.target['readOnly']) {
      this.contextMenuService.show.next({
        contextMenu: this.pasteSelectContextMenu,
        event: $event,
        item: $event.target,
      });
      $event.preventDefault();
      $event.stopPropagation();
    }
  }

}
