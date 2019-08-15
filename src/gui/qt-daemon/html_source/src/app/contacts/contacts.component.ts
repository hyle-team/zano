import { Component, OnInit, ViewChild, ElementRef } from '@angular/core';
import { Location } from '@angular/common';
import { VariablesService } from '../_helpers/services/variables.service';
import { BackendService } from '../_helpers/services/backend.service';

@Component({
  selector: 'app-contacts',
  templateUrl: './contacts.component.html',
  styleUrls: ['./contacts.component.scss']
})
export class ContactsComponent implements OnInit {
  calculatedWidth = [];
  @ViewChild('head') head: ElementRef;

  constructor(
    private location: Location,
    private variablesService: VariablesService,
    private backend: BackendService
  ) {}

  ngOnInit() {
    this.backend.getContactAlias();
  }

  delete(index: number) {
    if (this.variablesService.appPass) {
      this.variablesService.contacts.splice(index, 1);
      this.backend.storeSecureAppData();
    }
  }

  calculateWidth() {
    this.calculatedWidth = [];
    this.calculatedWidth.push(
      this.head.nativeElement.childNodes[0].clientWidth
    );
    this.calculatedWidth.push(
      this.head.nativeElement.childNodes[1].clientWidth +
        this.head.nativeElement.childNodes[2].clientWidth
    );
    this.calculatedWidth.push(
      this.head.nativeElement.childNodes[3].clientWidth
    );
    this.calculatedWidth.push(
      this.head.nativeElement.childNodes[4].clientWidth
    );
  }

<<<<<<< HEAD
  // openInBrowser(alias: string) {
  //   if (alias !== null) {
  //     this.backend.openUrlInBrowser(
  //       `explorer.zano.org/aliases/${alias.slice(1)}#modalOpen`
  //     );
  //   }
  // }
=======
  openInBrowser(alias: string) {
    if (alias !== null) {
      this.backend.openUrlInBrowser(
        `explorer.zano.org/aliases/${alias.slice(1)}`
      );
    }
  }
>>>>>>> 3ff1ce583e414436a973956284587d52e402f589

  back() {
    this.location.back();
  }
}
