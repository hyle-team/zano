import {Component, OnInit, OnDestroy, ViewChild, ElementRef, Renderer2} from '@angular/core';
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
  @ViewChild('copyButton') copy: ElementRef;

  constructor(
    private route: ActivatedRoute,
    private renderer: Renderer2,
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
    this.renderer.removeClass(this.copy.nativeElement, 'copy');
    this.renderer.addClass(this.copy.nativeElement, 'copied');
    window.setTimeout(() => {
      this.renderer.removeClass(this.copy.nativeElement, 'copied');
      this.renderer.addClass(this.copy.nativeElement, 'copy');
    }, 2000);
  }

  ngOnDestroy() {
    this.parentRouting.unsubscribe();
  }

}
