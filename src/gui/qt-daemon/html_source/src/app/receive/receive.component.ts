import {Component, OnInit, OnDestroy} from '@angular/core';
import QRCode from 'qrcode';
import {BackendService} from '../_helpers/services/backend.service';
import {VariablesService} from '../_helpers/services/variables.service';
import {ActivatedRoute} from '@angular/router';

@Component({
  selector: 'app-receive',
  templateUrl: './receive.component.html',
  styleUrls: ['./receive.component.scss']
})
export class ReceiveComponent implements OnInit, OnDestroy {
  qrImageSrc: string;
  parentRouting;

  constructor(
    private route: ActivatedRoute,
    private backend: BackendService,
    private variablesService: VariablesService
  ) {
  }

  ngOnInit() {
    this.parentRouting = this.route.parent.params.subscribe(() => {
      QRCode.toDataURL(this.variablesService.currentWallet.address, {
        width: 106,
        height: 106
      }).then(url => {
        this.qrImageSrc = url;
      }).catch(err => {
        console.error(err);
      });
    });
  }

  public copyAddress() {
    this.backend.setClipboard(this.variablesService.currentWallet.address);
  }

  ngOnDestroy() {
    this.parentRouting.unsubscribe();
  }

}
