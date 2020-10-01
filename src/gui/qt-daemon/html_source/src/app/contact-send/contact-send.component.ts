import { Component, OnInit, OnDestroy } from '@angular/core';
import { Location } from '@angular/common';
import { VariablesService } from '../_helpers/services/variables.service';
import { ActivatedRoute } from '@angular/router';


@Component({
  selector: 'app-contact-send',
  templateUrl: './contact-send.component.html',
  styleUrls: ['./contact-send.component.scss']
})
export class ContactSendComponent implements OnInit, OnDestroy {

  queryRouting;
  address;

  constructor(
    private location: Location,
    public variablesService: VariablesService,
    private route: ActivatedRoute
  ) { }

  ngOnInit() {
    this.queryRouting = this.route.queryParams.subscribe(params => {
      if (params.address) {
        this.address = params.address;
      }
    });
  }

  goToWallet(id) {
    this.variablesService.setCurrentWallet(id);
    this.variablesService.currentWallet.send_data['address'] = this.address;
  }

  back() {
    this.location.back();
  }

  ngOnDestroy() {
    this.queryRouting.unsubscribe();
  }

}
