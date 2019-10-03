import { BrowserModule } from '@angular/platform-browser';
import { NgModule } from '@angular/core';

import { AppRoutingModule } from './app-routing.module';

import { AppComponent } from './app.component';
import { LoginComponent } from './login/login.component';
import { SettingsComponent } from './settings/settings.component';
import { SidebarComponent } from './sidebar/sidebar.component';
import { MainComponent } from './main/main.component';
import { CreateWalletComponent } from './create-wallet/create-wallet.component';
import { OpenWalletComponent } from './open-wallet/open-wallet.component';
import { OpenWalletModalComponent } from './open-wallet-modal/open-wallet-modal.component';
import { RestoreWalletComponent } from './restore-wallet/restore-wallet.component';
import { SeedPhraseComponent } from './seed-phrase/seed-phrase.component';
import { WalletDetailsComponent } from './wallet-details/wallet-details.component';
import { AssignAliasComponent } from './assign-alias/assign-alias.component';
import { EditAliasComponent } from './edit-alias/edit-alias.component';
import { TransferAliasComponent } from './transfer-alias/transfer-alias.component';
import { WalletComponent } from './wallet/wallet.component';
import { SendComponent } from './send/send.component';
import { ReceiveComponent } from './receive/receive.component';
import { HistoryComponent } from './history/history.component';
import { ContractsComponent } from './contracts/contracts.component';
import { PurchaseComponent } from './purchase/purchase.component';
import { MessagesComponent } from './messages/messages.component';
import { TypingMessageComponent } from './typing-message/typing-message.component';
import { StakingComponent } from './staking/staking.component';

import { HttpClient, HttpClientModule } from '@angular/common/http';
import { TranslateLoader, TranslateModule } from '@ngx-translate/core';
import { TranslateHttpLoader } from '@ngx-translate/http-loader';
import { FormsModule, ReactiveFormsModule } from '@angular/forms';
import { NgSelectModule } from '@ng-select/ng-select';

import { BackendService } from './_helpers/services/backend.service';
import { ModalService } from './_helpers/services/modal.service';
import { MoneyToIntPipe } from './_helpers/pipes/money-to-int.pipe';
import { IntToMoneyPipe } from './_helpers/pipes/int-to-money.pipe';
import { HistoryTypeMessagesPipe } from './_helpers/pipes/history-type-messages.pipe';
import { ContractStatusMessagesPipe } from './_helpers/pipes/contract-status-messages.pipe';
import { ContractTimeLeftPipe } from './_helpers/pipes/contract-time-left.pipe';
import { SafeHTMLPipe } from './_helpers/pipes/safe-html.pipe';
import { TooltipDirective } from './_helpers/directives/tooltip.directive';
import { InputValidateDirective } from './_helpers/directives/input-validate/input-validate.directive';
import { StakingSwitchComponent } from './_helpers/directives/staking-switch/staking-switch.component';
import { ModalContainerComponent } from './_helpers/directives/modal-container/modal-container.component';
import { TransactionDetailsComponent } from './_helpers/directives/transaction-details/transaction-details.component';
import { ContextMenuModule } from 'ngx-contextmenu';
import { ChartModule, HIGHCHARTS_MODULES } from 'angular-highcharts';
import * as highcharts from 'highcharts';
import exporting from 'highcharts/modules/exporting.src';
import { ProgressContainerComponent } from './_helpers/directives/progress-container/progress-container.component';
import { InputDisableSelectionDirective } from './_helpers/directives/input-disable-selection/input-disable-selection.directive';
import { SendModalComponent } from './send-modal/send-modal.component';
import { ContactsComponent } from './contacts/contacts.component';
import { AddContactsComponent } from './add-contacts/add-contacts.component';
import { ContactSendComponent } from './contact-send/contact-send.component';
import { ExportImportComponent } from './export-import/export-import.component';
import { ConfirmModalComponent } from './_helpers/directives/confirm-modal/confirm-modal.component';

export function HttpLoaderFactory(httpClient: HttpClient) {
  return new TranslateHttpLoader(httpClient, './assets/i18n/', '.json');
}

import { PapaParseModule } from 'ngx-papaparse';

// import * as more from 'highcharts/highcharts-more.src';
// import * as exporting from 'highcharts/modules/exporting.src';
// import * as highstock from 'highcharts/modules/stock.src';

export function highchartsFactory() {
  // Default options.
  highcharts.setOptions({
    time: {
      useUTC: false
    }
  });

  return [exporting];
}

@NgModule({
  declarations: [
    AppComponent,
    LoginComponent,
    SettingsComponent,
    SidebarComponent,
    MainComponent,
    CreateWalletComponent,
    OpenWalletComponent,
    OpenWalletModalComponent,
    RestoreWalletComponent,
    SeedPhraseComponent,
    WalletDetailsComponent,
    AssignAliasComponent,
    EditAliasComponent,
    TransferAliasComponent,
    WalletComponent,
    SendComponent,
    ReceiveComponent,
    HistoryComponent,
    ContractsComponent,
    PurchaseComponent,
    MessagesComponent,
    StakingComponent,
    TypingMessageComponent,
    MoneyToIntPipe,
    IntToMoneyPipe,
    StakingSwitchComponent,
    HistoryTypeMessagesPipe,
    ContractStatusMessagesPipe,
    ContractTimeLeftPipe,
    TooltipDirective,
    InputValidateDirective,
    ModalContainerComponent,
    TransactionDetailsComponent,
    ProgressContainerComponent,
    InputDisableSelectionDirective,
    SendModalComponent,
    ContactsComponent,
    AddContactsComponent,
    ContactSendComponent,
    ExportImportComponent,
    SafeHTMLPipe,
    ConfirmModalComponent
  ],
  imports: [
    BrowserModule,
    AppRoutingModule,
    HttpClientModule,
    TranslateModule.forRoot({
      loader: {
        provide: TranslateLoader,
        useFactory: HttpLoaderFactory,
        deps: [HttpClient]
      }
    }),
    FormsModule,
    ReactiveFormsModule,
    NgSelectModule,
    ChartModule,
    PapaParseModule,
    ContextMenuModule.forRoot()
  ],
  providers: [
    BackendService,
    ModalService,
    MoneyToIntPipe,
    IntToMoneyPipe,
    { provide: HIGHCHARTS_MODULES, useFactory: highchartsFactory }
    // {provide: HIGHCHARTS_MODULES, useFactory: () => [ highstock, more, exporting ] }
  ],
  entryComponents: [
    ModalContainerComponent,
    SendModalComponent,
    ConfirmModalComponent
  ],
  bootstrap: [AppComponent]
})
export class AppModule { }
