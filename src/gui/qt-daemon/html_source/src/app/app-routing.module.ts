import { NgModule } from '@angular/core';
import { Routes, RouterModule } from '@angular/router';

// Components
import { MainComponent } from './main/main.component';
import { LoginComponent } from './login/login.component';
import { WalletComponent } from './wallet/wallet.component';
import { SendComponent } from './send/send.component';
import { ReceiveComponent } from './receive/receive.component';
import { HistoryComponent } from './history/history.component';
import { ContractsComponent } from './contracts/contracts.component';
import { PurchaseComponent } from './purchase/purchase.component';
import { MessagesComponent } from './messages/messages.component';
import { TypingMessageComponent } from './typing-message/typing-message.component';
import { StakingComponent } from './staking/staking.component';
import { SettingsComponent } from './settings/settings.component';
import { CreateWalletComponent } from './create-wallet/create-wallet.component';
import { OpenWalletComponent } from './open-wallet/open-wallet.component';
import { RestoreWalletComponent } from './restore-wallet/restore-wallet.component';
import { SeedPhraseComponent } from './seed-phrase/seed-phrase.component';
import { WalletDetailsComponent } from './wallet-details/wallet-details.component';

const routes: Routes = [
  {
    path: '',
    component: MainComponent
  },
  {
    path: 'main',
    component: MainComponent
  },
  {
    path: 'login',
    component: LoginComponent
  },
  {
    path: 'wallet/:id',
    component: WalletComponent,
    children: [
      {
        path: 'send',
        component: SendComponent
      },
      {
        path: 'receive',
        component: ReceiveComponent
      },
      {
        path: 'history',
        component: HistoryComponent
      },
      {
        path: 'contracts',
        component: ContractsComponent,
      },
      {
        path: 'purchase',
        component: PurchaseComponent
      },
      {
        path: 'purchase/:id',
        component: PurchaseComponent
      },
      {
        path: 'messages',
        component: MessagesComponent,
      },
      {
        path: 'messages/:id',
        component: TypingMessageComponent,
      },
      {
        path: 'staking',
        component: StakingComponent
      },
      {
        path: '',
        redirectTo: 'history',
        pathMatch: 'full'
      }
    ]
  },
  {
    path: 'create',
    component: CreateWalletComponent
  },
  {
    path: 'open',
    component: OpenWalletComponent
  },
  {
    path: 'restore',
    component: RestoreWalletComponent
  },
  {
    path: 'seed-phrase',
    component: SeedPhraseComponent
  },
  {
    path: 'details',
    component: WalletDetailsComponent
  },
  {
    path: 'settings',
    component: SettingsComponent
  },
  {
    path: '',
    redirectTo: '/',
    pathMatch: 'full'
  }
];

@NgModule({
  imports: [RouterModule.forRoot(routes)],
  exports: [RouterModule]
})


export class AppRoutingModule { }
